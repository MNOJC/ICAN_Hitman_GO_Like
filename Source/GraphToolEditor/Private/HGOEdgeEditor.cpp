// Fill out your copyright notice in the Description page of Project Settings.


#include "HGOEdgeEditor.h"
#include "EngineUtils.h"

// Sets default values
AHGOEdgeEditor::AHGOEdgeEditor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	EdgeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EdgeMesh"));
	EdgeMesh->SetupAttachment(Root);

	bIsEditorOnlyActor = true;              
}

// Called when the game starts or when spawned
void AHGOEdgeEditor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHGOEdgeEditor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

