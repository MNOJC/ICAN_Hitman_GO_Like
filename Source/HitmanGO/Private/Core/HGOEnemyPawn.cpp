// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/HGOEnemyPawn.h"
#include "Graph/HGOTacticalLevelGenerator.h"
#include "EngineUtils.h"
#include "Core/HGOTacticalTurnManager.h"

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

void AHGOEnemyPawn::ExecuteEnemyRotation()
{
	if (!GraphMovementComponent || !GraphMovementComponent->GetCurrentNode())
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] Cannot calculate rotation - no current node"));
		return;
	}

	// Récupérer la prochaine node dans le path
	int32 NextPathIndex = (CurrentPathIndex + 1) % MovementPathNodeIDs.Num();
	int32 NextNodeID = MovementPathNodeIDs[NextPathIndex];

	// Trouver la node correspondante
	AHGOTacticalLevelGenerator* Generator = nullptr;
	for (TActorIterator<AHGOTacticalLevelGenerator> GeneratorItr(GetWorld()); GeneratorItr; ++GeneratorItr)
	{
		Generator = *GeneratorItr;
		break;
	}

	if (!Generator)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No level generator found for rotation"));
		return;
	}

	UHGONodeGraphComponent* NextNode = nullptr;
	for (UHGONodeGraphComponent* NodeComp : Generator->NodeGraphs)
	{
		if (NodeComp && NodeComp->NodeData.NodeID == NextNodeID)
		{
			NextNode = NodeComp;
			break;
		}
	}

	if (!NextNode)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] Next node %d not found"), NextNodeID);
		return;
	}

	// Calculer la direction vers la prochaine node
	FVector CurrentPos = GraphMovementComponent->GetCurrentNode()->GetComponentLocation();
	FVector NextPos = NextNode->GetComponentLocation();
	FVector Direction = (NextPos - CurrentPos).GetSafeNormal();

	// Calculer la rotation nécessaire (seulement Yaw pour rotation horizontale)
	FRotator TargetRotation = Direction.Rotation();
	TargetRotation.Pitch = 0.0f; // Garder l'ennemi droit
	TargetRotation.Roll = 0.0f;

	NextRotation = TargetRotation;
	bIsRotating = true;

	UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Rotating to face node %d (Yaw: %.1f)"), 
		NextNodeID, TargetRotation.Yaw);
}

void AHGOEnemyPawn::UpdateEnemyRotation(float DeltaTime)
{
	if (bIsRotating)
	{
		FRotator TargetRotation = FMath::RInterpConstantTo(GetActorRotation(), NextRotation, DeltaTime, 360.f); 
		this->SetActorRotation(TargetRotation);

		if (TargetRotation.Equals(NextRotation, 1.0f))
		{
			bIsRotating = false;
			UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Rotation complete"));
			if (UWorld* World = GetWorld())
			{
				if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
				{
					TurnManager->RegisterActionCompleted();
				}
			}
		}
	}
}

// Called every frame
void AHGOEnemyPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateEnemyRotation(DeltaTime);
}

// Called to bind functionality to input
void AHGOEnemyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

