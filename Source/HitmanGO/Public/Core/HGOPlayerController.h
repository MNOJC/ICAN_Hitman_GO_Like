// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "HGOPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class HITMANGO_API AHGOPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	AHGOPlayerController();

	virtual void BeginPlay() override;

	// INPUTS SETUP
	virtual void SetupInputComponent() override;

	// INPUTS FUNCTIONS
	void Look(const FInputActionValue& Value);
	void CameraRotatePressed(const FInputActionValue& Value);
	void CameraRotateReleased(const FInputActionValue& Value);

private:

	// INPUTS MAPPING
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* PlayerMappingContext;

	//INPUTS ACTIONS
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* CameraRotateAction;

	// CAMERA ROTATION FLAG
	bool bRotateCamera = false;
	
};
