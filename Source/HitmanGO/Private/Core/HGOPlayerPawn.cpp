// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/HGOPlayerPawn.h"
#include "FMODBlueprintStatics.h"
#include "Core/HGOPlayerController.h"
#include "Core/HGOTacticalTurnManager.h"
#include "Core/HGOEnemyPawn.h"
#include "Graph/HGOTacticalLevelGenerator.h"
#include "Materials/MaterialExpressionFmod.h"

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
	BlockInput();
	
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AHGOTacticalLevelGenerator> GenItr(World); GenItr; ++GenItr)
		{
			AHGOTacticalLevelGenerator* Generator = *GenItr;
			if (Generator)
			{
				Generator->OnGraphAnimationCompleted.AddDynamic(this, &AHGOPlayerPawn::UnblockInput);
				Generator->OnBoardFlipAnimCompleted.AddDynamic(this, &AHGOPlayerPawn::BlockInput);
				Generator->OnSwitchWorldAnimCompleted.AddDynamic(this, &AHGOPlayerPawn::UnblockInput);
                
				break;
			}
		}

		if(AHGOGameMode* GM =  Cast<AHGOGameMode>(World->GetAuthGameMode()))
		{
			GM->OnSwitchWorldGraph.AddDynamic(this, &AHGOPlayerPawn::OnSwitchWorldTrigger);
		}
	}
}

void AHGOPlayerPawn::OnPawnClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{

	if (UWorld* World = GetWorld())
	{
		AHGOPlayerController* HGOController = Cast<AHGOPlayerController>(World->GetFirstPlayerController());
		if (HGOController)
		{
			HGOController->bPawnHovered = true;
			if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
			{
				if (!TurnManager->IsPlayerTurn())
				{
					GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Orange,
						TEXT("[Player] Not your turn"));
					return;
				}
			}

			UFMODBlueprintStatics::PlayEvent2D(GetWorld(), PlayerPawnGrabSound, true);
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
			HGOController->bPawnHovered = false;
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
					SetActorRotation(DefaultSpawnRotation);

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

void AHGOPlayerPawn::KillPlayer(bool KillPlayerFromOtherWorld)
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerPawn] Player has been killed!"));
	
	if (UWorld* World = GetWorld())
	{
		if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
		{
			TurnManager->StopGame();
		}
	}
	
	OnPlayerDeath.Broadcast(KillPlayerFromOtherWorld);
}

void AHGOPlayerPawn::CompleteLevel()
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerPawn] Level completed!"));
	
	OnLevelComplete.Broadcast();
	
}

void AHGOPlayerPawn::TriggerPlayerAbility()
{
	if (bInputBlocked)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red,
			TEXT("[Ability] Input blocked - Animation in progress"));
		return;
	}

	
	GEngine->AddOnScreenDebugMessage(
		-1, 3.f, FColor::Cyan,
		TEXT("[Ability] TriggerPlayerAbility called")
	);
	
	if (UWorld* World = GetWorld())
	{
		if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
		{
			if (!TurnManager->IsPlayerTurn())
			{
				GEngine->AddOnScreenDebugMessage(
					-1, 3.f, FColor::Red,
					TEXT("[Ability] FAILED - Not player's turn")
				);
				return;
			}
		}
	}

	GEngine->AddOnScreenDebugMessage(
		-1, 3.f, FColor::Green,
		TEXT("[Ability] ✓ Player turn confirmed")
	);
	
	if (CurrentAbilityCooldown > 0)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 3.f, FColor::Red,
			FString::Printf(TEXT("[Ability] FAILED - Cooldown remaining: %d turns"), CurrentAbilityCooldown)
		);
		return;
	}

	GEngine->AddOnScreenDebugMessage(
		-1, 3.f, FColor::Green,
		TEXT("[Ability] ✓ Cooldown ready")
	);
	
	if (!GraphMovementComponent || !GraphMovementComponent->GetCurrentNode())
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 3.f, FColor::Red,
			TEXT("[Ability] FAILED - No current node")
		);
		return;
	}
	
	AHGOEnemyPawn* TargetEnemy = nullptr;
	ENodeDirection AlignedDirection = ENodeDirection::None;

	if (!FindPushableEnemy(TargetEnemy, AlignedDirection))
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 3.f, FColor::Red,
			TEXT("[Ability] FAILED - No aligned enemy found")
		);
		return;
	}

	GEngine->AddOnScreenDebugMessage(
		-1, 3.f, FColor::Green,
		FString::Printf(TEXT("[Ability] ✓ Enemy aligned in direction: %s"), 
			*UEnum::GetValueAsString(AlignedDirection))
	);
	
	GEngine->AddOnScreenDebugMessage(
		-1, 3.f, FColor::Magenta,
		TEXT("[Ability] ═══ ABILITY ACTIVATED ═══")
	);
	
	CurrentAbilityCooldown = AbilityCooldownTurns;
	CheckAbilityAvailability();

	GEngine->AddOnScreenDebugMessage(
		-1, 3.f, FColor::Orange,
		FString::Printf(TEXT("[Ability] Cooldown set to %d turns"), CurrentAbilityCooldown)
	);
	

	UFMODBlueprintStatics::PlayEvent2D(GetWorld(), AbilitySound, true);
	TargetEnemy->PushEnemy(AlignedDirection);
	
}

void AHGOPlayerPawn::BlockInput()
{
	bInputBlocked = true;
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
		TEXT("[Player] ✗ INPUT BLOCKED ✗"));
	OnInputBlocked.Broadcast(true);
}

void AHGOPlayerPawn::UnblockInput()
{
	bInputBlocked = false;
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
		TEXT("[Player] ✓ INPUT UNLOCKED ✓"));
	OnInputBlocked.Broadcast(false);
}

void AHGOPlayerPawn::UpdateAbilityCooldown()
{
	if (CurrentAbilityCooldown > 0)
	{
		CurrentAbilityCooldown--;
		
		GEngine->AddOnScreenDebugMessage(
			-1, 2.f, FColor::Yellow,
			FString::Printf(TEXT("[Ability] Cooldown decreased: %d turns remaining"), CurrentAbilityCooldown)
		);

		CheckAbilityAvailability();
		OnAbilityCooldownUpdated.Broadcast(CurrentAbilityCooldown);
	}
	
}

void AHGOPlayerPawn::CheckAbilityAvailability()
{
	bool bWasAvailable = bAbilityAvailable;
	bAbilityAvailable = (CurrentAbilityCooldown == 0);

	if (bAbilityAvailable && !bWasAvailable)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 3.f, FColor::Green,
			TEXT("[Ability] ★ ABILITY NOW AVAILABLE ★")
		);
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), AbilityReadySound, true);
		OnAbilityBecameAvailable();
	}
	else if (!bAbilityAvailable && bWasAvailable)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 3.f, FColor::Red,
			TEXT("[Ability] ✗ ABILITY NOW UNAVAILABLE ✗")
		);
		OnAbilityBecameUnavailable();
	}
}

bool AHGOPlayerPawn::FindPushableEnemy(AHGOEnemyPawn*& OutEnemy, ENodeDirection& OutDirection) const
{
	OutEnemy = nullptr;
	OutDirection = ENodeDirection::None;

	if (!GraphMovementComponent || !GraphMovementComponent->GetCurrentNode())
	{
		return false;
	}
	
	if (CurrentAbilityCooldown > 0)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	
	for (TActorIterator<AHGOEnemyPawn> EnemyItr(World); EnemyItr; ++EnemyItr)
	{
		AHGOEnemyPawn* Enemy = *EnemyItr;
		if (!Enemy || !Enemy->GraphMovementComponent)
		{
			continue;
		}

		UHGONodeGraphComponent* EnemyNode = Enemy->GraphMovementComponent->GetCurrentNode();
		if (!EnemyNode)
		{
			continue;
		}
		
		if (GraphMovementComponent->bInUpsideDownWorld != Enemy->GraphMovementComponent->bInUpsideDownWorld)
		{
			continue;
		}
		
		ENodeDirection Direction = ENodeDirection::None;
		if (GraphMovementComponent->IsNodeInAlignedDirection(EnemyNode, Direction))
		{
			OutEnemy = Enemy;
			OutDirection = Direction;
			return true;
		}
	}

	return false;
}

void AHGOPlayerPawn::EvaluatePushReadyAtTurnStart()
{
	AHGOEnemyPawn* FoundEnemy = nullptr;
	ENodeDirection FoundDirection = ENodeDirection::None;

	bIsPushReadyThisTurn = FindPushableEnemy(FoundEnemy, FoundDirection);

	OnPlayerAlignedAndPushReady.Broadcast(bIsPushReadyThisTurn);

	UE_LOG(LogTemp, Log, TEXT("[Player] Push ready at turn start: %s"),
		bIsPushReadyThisTurn ? TEXT("TRUE") : TEXT("FALSE"));

	if (bIsPushReadyThisTurn)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.f,
			FColor::Green,
			FString::Printf(TEXT("[Player] Push READY (%s)"),
				*UEnum::GetValueAsString(FoundDirection))
		);
	}
}

void AHGOPlayerPawn::OnSwitchWorldTrigger(bool bToUpsideDown)
{
	BlockInput();
}

void AHGOPlayerPawn::OnAbilityBecameAvailable_Implementation()
{

}

void AHGOPlayerPawn::OnAbilityBecameUnavailable_Implementation()
{

}
