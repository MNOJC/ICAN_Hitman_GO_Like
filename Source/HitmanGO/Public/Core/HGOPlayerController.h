// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
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

	// INPUTS SETUP
	virtual void SetupInputComponent() override;

	// INPUTS FUNCTIONS
	void Look(const FInputActionValue& Value);
	void CameraRotatePressed(const FInputActionValue& Value);
	void CameraRotateReleased(const FInputActionValue& Value);
	void PawnPressed(const FInputActionValue& Value);
	void PawnReleased(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	float SwipeThreshold = 2.0f;

	FRotator TargetCameraRotation;
	

private:

	// INPUTS MAPPING
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* PlayerMappingContext;

	//INPUTS ACTIONS
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MouseInteractionAction;
	
	
	// CAMERA ROTATION FLAG
	bool bRotateCamera = false;

	// PAWN SELECTION FLAG
	bool bPawnSelected = false;

	FVector2D SwipeStartPosition;
	FVector2D SwipeDelta;

	// SWIPE DIRECTION CALCULATION
	ENodeDirection CalculateSwipeDirection(FVector2D Delta);
};
