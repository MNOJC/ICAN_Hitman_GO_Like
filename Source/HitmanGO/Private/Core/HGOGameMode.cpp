// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/HGOGameMode.h"
#include "HGOLog.h"
#include "Camera/HGOCamera.h"

void AHGOGameMode::BeginPlay()
{
	Super::BeginPlay();

	InitLevelCamera();
}

void AHGOGameMode::InitLevelCamera()
{
	if (!LevelCameraClass)
		return;
	
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		AHGOCamera* LevelCamera = World->SpawnActor<AHGOCamera>(LevelCameraClass, SpawnParams);
		LevelCamera->SetActorLocation(FVector(300.f, 0.f, 0.f));
		
		if (!LevelCamera) return;

		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			UE_LOG(LogHitmanGO, Warning, TEXT("Setting view target to level camera"));
			PC->SetViewTarget(LevelCamera);
		}
	}
}
