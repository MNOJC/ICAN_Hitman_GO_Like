// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/HGOPlayerController.h"

AHGOPlayerController::AHGOPlayerController()
{
	SetShowMouseCursor(true);
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

void AHGOPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHGOPlayerController::Look);
		EIC->BindAction(CameraRotateAction, ETriggerEvent::Started, this, &AHGOPlayerController::CameraRotatePressed);
		EIC->BindAction(CameraRotateAction, ETriggerEvent::Completed, this, &AHGOPlayerController::CameraRotateReleased);
	}
}

void AHGOPlayerController::Look(const FInputActionValue& Value)
{
	if (!bRotateCamera)
		return;

	const FVector2D LookAxis = Value.Get<FVector2D>();
	
	if (AActor* ViewTarget = GetViewTarget())
	{
		FRotator NewRotation = ViewTarget->GetActorRotation();
		NewRotation.Yaw += LookAxis.X;
		NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch + LookAxis.Y,-80.f,80.f);

		ViewTarget->SetActorRotation(NewRotation);
	}
}

void AHGOPlayerController::CameraRotatePressed(const FInputActionValue& Value)
{
	bRotateCamera = true;
}

void AHGOPlayerController::CameraRotateReleased(const FInputActionValue& Value)
{
	bRotateCamera = false;
}
