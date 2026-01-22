// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/HGOPlayerController.h"
#include "EngineUtils.h"

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
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(PlayerMappingContext, 0);
		}
	}
}

void AHGOPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (AActor* ViewTarget = GetViewTarget())
	{	
		ViewTarget->SetActorRotation(FMath::RInterpTo(ViewTarget->GetActorRotation(), TargetCameraRotation, DeltaTime, 10.f));
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
		
		FRotator NewRotation = ViewTarget->GetActorRotation();
		NewRotation.Yaw += LookAxis.X;
		NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch + LookAxis.Y,-90.f,-10.f);
		
		TargetCameraRotation = NewRotation;
	}
}

void AHGOPlayerController::CameraRotatePressed(const FInputActionValue& Value)
{
	if (bPawnSelected)
	return;
	
	bRotateCamera = true;
}

void AHGOPlayerController::CameraRotateReleased(const FInputActionValue& Value)
{
	bRotateCamera = false;
	UE_LOG(LogTemp, Warning, TEXT("Camera Rotate Released"));
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
	UE_LOG(LogTemp, Warning, TEXT("Pawn Released"));
	
	if (bPawnSelected)
	{
		float SwipeLength = SwipeDelta.Length();

		if (SwipeLength >= SwipeThreshold)
		{
			ENodeDirection SwipeDirection = CalculateSwipeDirection(SwipeDelta);

			if (SwipeDirection != ENodeDirection::None)
			{
				APawn* ControlledPawn = GetPawn();
				
				if (ControlledPawn)
				{
					UHGOGraphMovementComponent* MovementComp = ControlledPawn->FindComponentByClass<UHGOGraphMovementComponent>();

					if (MovementComp)
					{
						MovementComp->TryMoveInDirection(SwipeDirection);
					}
				}
			}
		}

		GetPawn()->SetActorLocation(StartPawnLocationBeforeGrab);
		GetPawn()->SetActorRotation(FRotator::ZeroRotator);
	}

	bPawnSelected = false;
}

void AHGOPlayerController::PawnGrabbed(const FInputActionValue& Value)
{
	if (bPawnSelected)
	{
		GetPawn()->SetActorLocation(StartPawnLocationBeforeGrab + FVector(0, 0, 15.0f));
        
		FRotator TiltRotation = FRotator::ZeroRotator;
        
		if (SwipeDelta.Length() > 0.0f)
		{
			ENodeDirection SwipeDirection = CalculateSwipeDirection(SwipeDelta);
            
			float MaxTiltAngle = 25.0f;
			
			switch (SwipeDirection)
			{
			case ENodeDirection::North:
				TiltRotation.Pitch = -MaxTiltAngle; 
				break;
                    
			case ENodeDirection::South:
				TiltRotation.Pitch = MaxTiltAngle; 
				break;
                    
			case ENodeDirection::East:
				TiltRotation.Roll = MaxTiltAngle; 
				break;
                    
			case ENodeDirection::West:
				TiltRotation.Roll = -MaxTiltAngle;
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
