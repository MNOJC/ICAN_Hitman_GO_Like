// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/HGOPlayerPawn.h"
#include "Core/HGOPlayerController.h"
#include "Core/HGOTacticalTurnManager.h"
#include "Core/HGOEnemyPawn.h"
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
    
	// Bloquer l'input au démarrage
	BlockInput();
    
	// Bind au delegate du level generator pour débloquer après l'animation initiale
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AHGOTacticalLevelGenerator> GenItr(World); GenItr; ++GenItr)
		{
			AHGOTacticalLevelGenerator* Generator = *GenItr;
			if (Generator)
			{
				// Débloquer après l'animation initiale
				Generator->OnGraphAnimationCompleted.AddDynamic(this, &AHGOPlayerPawn::UnblockInput);
                
				// Bloquer pendant les switchs de monde
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
	// Bloquer si input désactivé
	if (bInputBlocked)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red,
			TEXT("[Player] Input blocked - Animation in progress"));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		AHGOPlayerController* HGOController = Cast<AHGOPlayerController>(World->GetFirstPlayerController());
		if (HGOController)
		{
			if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
			{
				if (!TurnManager->IsPlayerTurn())
				{
					GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Orange,
						TEXT("[Player] Not your turn"));
					return;
				}
			}
            
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
	
	// Stopper le système de tour immédiatement pour bloquer tout mouvement ennemi
	if (UWorld* World = GetWorld())
	{
		if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
		{
			TurnManager->StopGame();
		}
	}

	// Broadcast le delegate pour notifier les blueprints
	OnPlayerDeath.Broadcast();
}

void AHGOPlayerPawn::CompleteLevel()
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerPawn] Level completed!"));
	
	// Broadcast le delegate pour notifier les blueprints
	OnLevelComplete.Broadcast();
	
	// TODO: Ajouter des effets visuels, animations, son, transition vers le prochain niveau
}

void AHGOPlayerPawn::TriggerPlayerAbility()
{
	// Bloquer si input désactivé
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

	// CONDITION 1: Vérifier que c'est le tour du joueur
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

	// CONDITION 2: Vérifier le cooldown
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

	// CONDITION 3: Trouver un ennemi aligné
	if (!GraphMovementComponent || !GraphMovementComponent->GetCurrentNode())
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 3.f, FColor::Red,
			TEXT("[Ability] FAILED - No current node")
		);
		return;
	}

	// Parcourir tous les ennemis pour trouver un aligné
	AHGOEnemyPawn* TargetEnemy = nullptr;
	ENodeDirection AlignedDirection = ENodeDirection::None;

	for (TActorIterator<AHGOEnemyPawn> EnemyItr(GetWorld()); EnemyItr; ++EnemyItr)
	{
		AHGOEnemyPawn* Enemy = *EnemyItr;
		if (!Enemy || !Enemy->GraphMovementComponent)
			continue;

		UHGONodeGraphComponent* EnemyNode = Enemy->GraphMovementComponent->GetCurrentNode();
		if (!EnemyNode)
			continue;

		// Vérifier même monde
		if (GraphMovementComponent->bInUpsideDownWorld != Enemy->GraphMovementComponent->bInUpsideDownWorld)
			continue;

		// Vérifier alignement
		ENodeDirection Direction;
		if (GraphMovementComponent->IsNodeInAlignedDirection(EnemyNode, Direction))
		{
			TargetEnemy = Enemy;
			AlignedDirection = Direction;
			break; // Premier ennemi trouvé
		}
	}

	if (!TargetEnemy)
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

	// TOUTES LES CONDITIONS RÉUNIES - ACTIVER L'ABILITY
	GEngine->AddOnScreenDebugMessage(
		-1, 3.f, FColor::Magenta,
		TEXT("[Ability] ═══ ABILITY ACTIVATED ═══")
	);

	// Activer le cooldown
	CurrentAbilityCooldown = AbilityCooldownTurns;
	CheckAbilityAvailability();

	GEngine->AddOnScreenDebugMessage(
		-1, 3.f, FColor::Orange,
		FString::Printf(TEXT("[Ability] Cooldown set to %d turns"), CurrentAbilityCooldown)
	);

	// Appeler PushEnemy sur l'ennemi
	TargetEnemy->PushEnemy(AlignedDirection);
}

void AHGOPlayerPawn::BlockInput()
{
	bInputBlocked = true;
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
		TEXT("[Player] ✗ INPUT BLOCKED ✗"));
}

void AHGOPlayerPawn::UnblockInput()
{
	bInputBlocked = false;
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
		TEXT("[Player] ✓ INPUT UNLOCKED ✓"));
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
		// L'ability vient de devenir disponible
		GEngine->AddOnScreenDebugMessage(
			-1, 3.f, FColor::Green,
			TEXT("[Ability] ★ ABILITY NOW AVAILABLE ★")
		);
		OnAbilityBecameAvailable();
	}
	else if (!bAbilityAvailable && bWasAvailable)
	{
		// L'ability vient de devenir indisponible
		GEngine->AddOnScreenDebugMessage(
			-1, 3.f, FColor::Red,
			TEXT("[Ability] ✗ ABILITY NOW UNAVAILABLE ✗")
		);
		OnAbilityBecameUnavailable();
	}
}

void AHGOPlayerPawn::OnSwitchWorldTrigger(bool bToUpsideDown)
{
	BlockInput();
}

void AHGOPlayerPawn::OnAbilityBecameAvailable_Implementation()
{
	// Implémentation par défaut vide - à override dans Blueprint
}

void AHGOPlayerPawn::OnAbilityBecameUnavailable_Implementation()
{
	// Implémentation par défaut vide - à override dans Blueprint
}
