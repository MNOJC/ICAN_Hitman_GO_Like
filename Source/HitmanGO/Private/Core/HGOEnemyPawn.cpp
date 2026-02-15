// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/HGOEnemyPawn.h"
#include "Graph/HGOTacticalLevelGenerator.h"
#include "EngineUtils.h"

// Sets default values
AHGOEnemyPawn::AHGOEnemyPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	EnemyMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyMeshComponent"));
	EnemyMeshComponent->SetupAttachment(SceneRoot);

	GraphMovementComponent = CreateDefaultSubobject<UHGOGraphMovementComponent>(TEXT("GraphMovementComponent"));
}

// Called when the game starts or when spawned
void AHGOEnemyPawn::BeginPlay()
{
	Super::BeginPlay();
	
	InitEnemyPosition();
}

void AHGOEnemyPawn::InitEnemyPosition()
{
	if (MovementPathNodeIDs.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No movement path defined!"));
		return;
	}

	// Find the level generator
	AHGOTacticalLevelGenerator* Generator = nullptr;
	for (TActorIterator<AHGOTacticalLevelGenerator> GeneratorItr(GetWorld()); GeneratorItr; ++GeneratorItr)
	{
		Generator = *GeneratorItr;
		break;
	}

	if (!Generator)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No level generator found!"));
		return;
	}

	// Find the start node
	int32 StartNodeID = MovementPathNodeIDs[0];
	UHGONodeGraphComponent* StartNode = nullptr;

	for (UHGONodeGraphComponent* NodeComp : Generator->NodeGraphs)
	{
		if (NodeComp && NodeComp->NodeData.NodeID == StartNodeID)
		{
			StartNode = NodeComp;
			break;
		}
	}

	if (StartNode)
	{
		GraphMovementComponent->SetCurrentNode(StartNode);
		UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Initialized at node %d"), StartNodeID);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] Could not find start node %d"), StartNodeID);
	}
}

void AHGOEnemyPawn::ExecuteEnemyMove()
{
	if (MovementPathNodeIDs.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No movement path defined!"));
		return;
	}

	if (!GraphMovementComponent || !GraphMovementComponent->GetCurrentNode())
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No current node!"));
		return;
	}

	// Increment to next path index (loop with modulo)
	CurrentPathIndex = (CurrentPathIndex + 1) % MovementPathNodeIDs.Num();
	int32 TargetNodeID = MovementPathNodeIDs[CurrentPathIndex];

	UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Moving to next node in path: %d (index %d/%d)"), 
		TargetNodeID, CurrentPathIndex, MovementPathNodeIDs.Num() - 1);

	// Try to move using GraphMovementComponent
	if (GraphMovementComponent->TryMoveToNodeID(TargetNodeID))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Successfully started move to node %d"), TargetNodeID);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] Failed to move to node %d"), TargetNodeID);
	}
}

// Called every frame
void AHGOEnemyPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AHGOEnemyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

