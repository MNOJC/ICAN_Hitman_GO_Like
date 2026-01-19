// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/HGOCamera.h"

AHGOCamera::AHGOCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
	SpringArm->TargetArmLength = 800.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
}


void AHGOCamera::BeginPlay()
{
	Super::BeginPlay();
	
}

void AHGOCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

