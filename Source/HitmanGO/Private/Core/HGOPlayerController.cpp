// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/HGOPlayerController.h"
#include "EngineUtils.h"

namespace
{
	float SnapYawToNearest90(float Yaw)
	{
		return FMath::RoundToFloat(Yaw / 90.0f) * 90.0f;
	}

	float GetYawFromSwipeDirection(ENodeDirection Direction)
	{
		switch (Direction)
		{
		case ENodeDirection::North:
			return 180.0f;

		case ENodeDirection::South:
			return 0.0f;

		case ENodeDirection::East:
			return -90.0f;

		case ENodeDirection::West:
			return 90.0f;

		default:
			return 0.0f;
		}
	}
}

AHGOPlayerController::AHGOPlayerController()
{
	SetShowMouseCursor(true);
	bEnableClickEvents = true;
}

void AHGOPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(PlayerMappingContext, 0);
		}
	}

	// Initialiser la caméra avec la rotation par défaut
	if (AActor* ViewTarget = GetViewTarget())
	{
		ViewTarget->SetActorRotation(DefaultCameraRotation);
		TargetCameraRotation = DefaultCameraRotation;
	}
}

void AHGOPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (AActor* ViewTarget = GetViewTarget())
	{
		// Si on reset la caméra, interpoler vers DefaultCameraRotation
		if (bResetCamera)
		{
			TargetCameraRotation = FMath::RInterpConstantTo(
				TargetCameraRotation, 
				DefaultCameraRotation, 
				DeltaTime, 
				CameraResetSpeed * 100.0f
			);

			// Vérifier si on a atteint la rotation par défaut
			if (TargetCameraRotation.Equals(DefaultCameraRotation, 0.1f))
			{
				TargetCameraRotation = DefaultCameraRotation;
				bResetCamera = false;
			}
		}

		// Interpoler la rotation de la caméra
		ViewTarget->SetActorRotation(FMath::RInterpTo(
			ViewTarget->GetActorRotation(), 
			TargetCameraRotation, 
			DeltaTime, 
			10.f
		));
	}
}

void AHGOPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHGOPlayerController::Look);
		EIC->BindAction(MouseInteractionAction, ETriggerEvent::Started, this, &AHGOPlayerController::CameraRotatePressed);
		EIC->BindAction(MouseInteractionAction, ETriggerEvent::Triggered, this, &AHGOPlayerController::PawnGrabbed);
		EIC->BindAction(MouseInteractionAction, ETriggerEvent::Completed, this, &AHGOPlayerController::CameraRotateReleased);
	}
}

void AHGOPlayerController::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>() * 10.0f;
	
	if (bPawnSelected)
	{
		SwipeDelta += LookAxis;
		return;
	}
	
	if (!bRotateCamera)
		return;

	
	if (AActor* ViewTarget = GetViewTarget())
	{
		// Annuler le reset si l'utilisateur bouge la caméra
		bResetCamera = false;
		
		FRotator NewRotation = ViewTarget->GetActorRotation();
		NewRotation.Yaw += LookAxis.X;
		NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch + LookAxis.Y, CameraPitchMin, CameraPitchMax);
		
		TargetCameraRotation = NewRotation;
	}
}

void AHGOPlayerController::CameraRotatePressed(const FInputActionValue& Value)
{
	if (bPawnSelected)
		return;
	if(bPawnHovered)
		return;
	
	bRotateCamera = true;
	bResetCamera = false; // Annuler le reset si on commence à tourner
}

void AHGOPlayerController::CameraRotateReleased(const FInputActionValue& Value)
{
	bRotateCamera = false;
	
	// Démarrer le reset vers la position par défaut
	bResetCamera = true;
	
	PawnReleased(FInputActionValue());
}

void AHGOPlayerController::PawnPressed(const FInputActionValue& Value)
{
	StartPawnLocationBeforeGrab = GetPawn()->GetActorLocation();
	bPawnSelected = true;
	bRotateCamera = false;
	SwipeDelta = FVector2D::ZeroVector;
}

void AHGOPlayerController::PawnReleased(const FInputActionValue& Value)
{
	if (bPawnSelected)
	{
		APawn* ControlledPawn = GetPawn();
		ENodeDirection SwipeDirection = ENodeDirection::None;

		float SwipeLength = SwipeDelta.Length();

		if (SwipeLength >= SwipeThreshold)
		{
			SwipeDirection = CalculateSwipeDirection(SwipeDelta);

			if (SwipeDirection != ENodeDirection::None && ControlledPawn)
			{
				if (UHGOGraphMovementComponent* MovementComp = ControlledPawn->FindComponentByClass<UHGOGraphMovementComponent>())
				{
					MovementComp->TryMoveInDirection(SwipeDirection);
				}
			}
		}

		if (ControlledPawn)
		{
			ControlledPawn->SetActorLocation(StartPawnLocationBeforeGrab);

			float TargetYaw = ControlledPawn->GetActorRotation().Yaw;

			// Si on a un swipe valide, on s'aligne sur la direction du swipe
			if (SwipeDirection != ENodeDirection::None)
			{
				TargetYaw = GetYawFromSwipeDirection(SwipeDirection);
			}
			// Sinon, on snap juste au multiple de 90° le plus proche
			else
			{
				TargetYaw = SnapYawToNearest90(TargetYaw);
			}

			ControlledPawn->SetActorRotation(FRotator(0.0f, TargetYaw, 0.0f));
		}
	}

	bPawnSelected = false;
	bPawnHovered = false;
}

void AHGOPlayerController::PawnGrabbed(const FInputActionValue& Value)
{
	if (bPawnSelected)
	{
		GetPawn()->SetActorLocation(StartPawnLocationBeforeGrab + FVector(0, 0, 2.0f));
        
		FRotator TiltRotation = FRotator::ZeroRotator;
        
		if (SwipeDelta.Length() > 0.0f)
		{
			ENodeDirection SwipeDirection = CalculateSwipeDirection(SwipeDelta);
            
			float MaxTiltAngle = 25.0f;
			
			switch (SwipeDirection)
			{
			case ENodeDirection::North:
				TiltRotation.Pitch = MaxTiltAngle;
				TiltRotation.Yaw = 180.0f; // Faire face à la direction du swipe
				break;
                    
			case ENodeDirection::South:
				TiltRotation.Pitch = MaxTiltAngle;
				TiltRotation.Yaw = 0.0f; // Faire face à la direction du swipe
				break;
                    
			case ENodeDirection::East:
				TiltRotation.Pitch = MaxTiltAngle;
				TiltRotation.Yaw = -90.0f; // Faire face à la direction du swipe
				break;
                    
			case ENodeDirection::West:
				TiltRotation.Pitch = MaxTiltAngle;
				TiltRotation.Yaw = 90.0f; // Faire face à la direction du swipe
				break;
                    
			default:
				break;
			}
			
		}
		
        
		GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), TiltRotation, GetWorld()->GetDeltaSeconds(), 10.0f));
	}
}

ENodeDirection AHGOPlayerController::CalculateSwipeDirection(FVector2D Delta)
{
	Delta.Normalize();
	
	float Angle = FMath::Atan2(Delta.Y, Delta.X) * (180.0f / PI);
	Angle = Angle - GetViewTarget()->GetActorRotation().Yaw;
	
	if (Angle < 0)
		Angle += 360.0f;
	
	if (Angle >= 45.0f && Angle < 135.0f)
	{
		return ENodeDirection::North;
	}
	else if (Angle >= 135.0f && Angle < 225.0f)
	{
		return ENodeDirection::West; 
	}
	else if (Angle >= 225.0f && Angle < 315.0f)
	{
		return ENodeDirection::South;
	}
	else
	{
		return ENodeDirection::East;
	}
}
