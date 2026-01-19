// Fill out your copyright notice in the Description page of Project Settings.


#include "Graph/HGOEdgeGraph.h"

// Sets default values
AHGOEdgeGraph::AHGOEdgeGraph()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	EdgeMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EdgeMeshComponent"));
	EdgeMeshComponent->SetupAttachment(SceneRoot);

}

// Called when the game starts or when spawned
void AHGOEdgeGraph::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHGOEdgeGraph::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

