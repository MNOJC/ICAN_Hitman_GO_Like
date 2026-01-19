// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Camera/CameraActor.h"
#include "HGOGameMode.generated.h"

/**
 * 
 */
UCLASS()
class HITMANGO_API AHGOGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

	// Level Camera Class
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Classes")
	TSubclassOf<AActor> LevelCameraClass;
	
private:

	// Initialize Level Camera
	void InitLevelCamera();
};
