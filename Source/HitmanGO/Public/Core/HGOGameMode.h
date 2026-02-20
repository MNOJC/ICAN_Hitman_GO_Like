// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Camera/CameraActor.h"
#include "HGOGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSwitchWorldGraph, bool, bToUpsidedown);

UCLASS()
class HITMANGO_API AHGOGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Classes")
	TSubclassOf<AActor> LevelCameraClass;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "World")
	FOnSwitchWorldGraph OnSwitchWorldGraph;
	
private:
	
	void InitLevelCamera();
};
