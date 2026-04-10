// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "FMODEvent.h"
#include "FMODBlueprintStatics.h"
#include "Core/HGOGraphMovementComponent.h"
#include "Enumeration/GraphDataEnumeration.h"
#include "GameFramework/PlayerController.h"
#include "HGOPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class HITMANGO_API AHGOPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AHGOPlayerController();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupInputComponent() override;
	
	void Look(const FInputActionValue& Value);
	void CameraRotatePressed(const FInputActionValue& Value);
	void CameraRotateReleased(const FInputActionValue& Value);
	void PawnPressed(const FInputActionValue& Value);
	void PawnReleased(const FInputActionValue& Value);
	void PawnGrabbed(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	float SwipeThreshold = 2.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FRotator DefaultCameraRotation = FRotator(-45.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraPitchMin = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraPitchMax = -10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraResetSpeed = 5.0f;

	FRotator TargetCameraRotation;
	bool bPawnHovered = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraYawMin = -80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraYawMax = 80.0f;

private:
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* PlayerMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MouseInteractionAction;
	
	bool bRotateCamera = false;
	bool bResetCamera = false;
	
	bool bPawnSelected = false;

	FVector2D SwipeStartPosition;
	FVector2D SwipeDelta;
	
	ENodeDirection CalculateSwipeDirection(FVector2D Delta);

	FVector StartPawnLocationBeforeGrab;
};
