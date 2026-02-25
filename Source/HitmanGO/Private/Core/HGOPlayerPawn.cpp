// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/HGOPlayerPawn.h"
#include "Core/HGOPlayerController.h"
#include "Core/HGOTacticalTurnManager.h"
#include "Graph/HGOTacticalLevelGenerator.h"

// Sets default values
AHGOPlayerPawn::AHGOPlayerPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	PlayerMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerMeshComponent"));
	PlayerMeshComponent->SetupAttachment(SceneRoot);

	CollisionSwipeComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionSwipeComponent"));
	CollisionSwipeComponent->SetupAttachment(SceneRoot);
	CollisionSwipeComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSwipeComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSwipeComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GraphMovementComponent = CreateDefaultSubobject<UHGOGraphMovementComponent>(TEXT("GraphMovementComponent"));

}

// Called when the game starts or when spawned
void AHGOPlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	CollisionSwipeComponent->OnClicked.AddDynamic(this, &AHGOPlayerPawn::OnPawnClicked);
	CollisionSwipeComponent->OnReleased.AddDynamic(this, &AHGOPlayerPawn::OnPawnReleased);
	
	InitPawnPosition();
}

void AHGOPlayerPawn::OnPawnClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	if (UWorld* World = GetWorld())
	{
		AHGOPlayerController* HGOController = Cast<AHGOPlayerController>(World->GetFirstPlayerController());
		if (HGOController)
		{
			if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
			{
				if (!TurnManager->IsPlayerTurn())
					return;
				
			}
			//UE_LOG(LogTemp, Warning, TEXT("Pawn Clicked"));
			HGOController->PawnPressed(FInputActionValue());
		}
	}
}

void AHGOPlayerPawn::OnPawnReleased(UPrimitiveComponent* TouchedComponent, FKey ButtonReleased)
{
	if (UWorld* World = GetWorld())
	{
		AHGOPlayerController* HGOController = Cast<AHGOPlayerController>(World->GetFirstPlayerController());
		if (HGOController)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Pawn Released"));
			HGOController->PawnReleased(FInputActionValue());
		}
	}
}

// Called every frame
void AHGOPlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AHGOPlayerPawn::InitPawnPosition()
{
	TActorIterator<AHGOTacticalLevelGenerator> LevelGeneratorItr(GetWorld());
	if (LevelGeneratorItr)
	{
		AHGOTacticalLevelGenerator* LevelGenerator = *LevelGeneratorItr;
		if (LevelGenerator && LevelGenerator->LevelData)
		{
			const TArray<FNodeData>& Nodes = LevelGenerator->LevelData->Nodes;
			for (const FNodeData& Node : Nodes)
			{
				if (Node.NodeType == ENodeType::Start)
				{
					FVector SpawnLocation = LevelGenerator->GetActorLocation() + Node.Position;
					SetActorLocation(SpawnLocation);

					UHGONodeGraphComponent* StartNode = nullptr;
					for (UHGONodeGraphComponent* GraphNode : LevelGenerator->NodeGraphs)
					{
						if (GraphNode && GraphNode->NodeData.NodeID == Node.NodeID)
						{
							StartNode = GraphNode;
							break;
						}
					}
					
					GraphMovementComponent->SetCurrentNode(StartNode);
					return;
				}
			}
		}
	}
}

void AHGOPlayerPawn::KillPlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerPawn] Player has been killed!"));
	
	// Broadcast le delegate pour notifier les blueprints
	OnPlayerDeath.Broadcast();
	
	// TODO: Ajouter des effets visuels, animations, son, etc.
}

void AHGOPlayerPawn::CompleteLevel()
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerPawn] Level completed!"));
	
	// Broadcast le delegate pour notifier les blueprints
	OnLevelComplete.Broadcast();
	
	// TODO: Ajouter des effets visuels, animations, son, transition vers le prochain niveau
}
