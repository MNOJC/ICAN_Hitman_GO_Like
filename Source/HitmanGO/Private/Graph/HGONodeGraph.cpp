// Fill out your copyright notice in the Description page of Project Settings.


#include "Graph/HGONodeGraph.h"

// Sets default values
AHGONodeGraph::AHGONodeGraph()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	NodeMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeMeshComponent"));
	NodeMeshComponent->SetupAttachment(SceneRoot);
}

AHGONodeGraph* AHGONodeGraph::GetNodeInDirection(ENodeDirection Direction)
{
	if (AHGONodeGraph** FoundNode = ConnectedNodes.Find(Direction))
	{
		return *FoundNode;
	}
	return nullptr;
}

bool AHGONodeGraph::CanMoveInDirection(ENodeDirection Direction)
{
	return ConnectedNodes.Contains(Direction);
}

// Called when the game starts or when spawned
void AHGONodeGraph::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHGONodeGraph::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

