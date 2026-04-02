// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/HGOGraphMovementComponent.h"
#include "Core/HGOTacticalTurnManager.h"
#include "EngineUtils.h"
#include "Core/HGOEnemyPawn.h"
#include "Core/HGOPlayerPawn.h"
#include "Graph/HGOTacticalLevelGenerator.h"

// Sets default values for this component's properties
UHGOGraphMovementComponent::UHGOGraphMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bIsMoving = false;
	MovementProgress = 0.0f;
}

bool UHGOGraphMovementComponent::TryMoveInDirection(ENodeDirection Direction)
{
	// Check if we can move
	if (bIsMoving)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Movement] Already moving, ignoring input"));
		return false;
	}
	
	if (!CurrentNode)
	{
		UE_LOG(LogTemp, Error, TEXT("[Movement] No current node!"));
		return false;
	}
	
	// Find next node
	UHGONodeGraphComponent* NextNode = CurrentNode->GetNodeInDirection(Direction);

	if (!NextNode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Movement] No node in that direction"));
		return false;
	}
	
	// Start movement
	TargetNode = NextNode;
	bIsMoving = true;
	MovementProgress = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("[Movement] Starting move from node %d to node %d"), 
		CurrentNode->NodeData.NodeID, TargetNode->NodeData.NodeID);

	NotifyMovementStarted();

	return true;
}

bool UHGOGraphMovementComponent::TryMoveToNodeID(int32 TargetNodeID)
{
	// Check if we can move
	if (bIsMoving)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Movement] Already moving, ignoring request"));
		return false;
	}
	
	if (!CurrentNode)
	{
		UE_LOG(LogTemp, Error, TEXT("[Movement] No current node!"));
		return false;
	}

	// Find the target node by ID
	UHGONodeGraphComponent* TargetNodeComp = nullptr;
	
	AHGOTacticalLevelGenerator* Generator = nullptr;
	for (TActorIterator<AHGOTacticalLevelGenerator> GeneratorItr(GetWorld()); GeneratorItr; ++GeneratorItr)
	{
		Generator = *GeneratorItr;
		break;
	}

	if (!Generator)
	{
		UE_LOG(LogTemp, Error, TEXT("[Movement] No level generator found!"));
		return false;
	}

	// Search for the target node
	for (UHGONodeGraphComponent* NodeComp : Generator->NodeGraphs)
	{
		if (NodeComp && NodeComp->NodeData.NodeID == TargetNodeID)
		{
			TargetNodeComp = NodeComp;
			break;
		}
	}

	if (!TargetNodeComp)
	{
		UE_LOG(LogTemp, Error, TEXT("[Movement] Target node %d not found!"), TargetNodeID);
		return false;
	}

	// Check if target node is adjacent to current node
	TArray<ENodeDirection> AllDirections = {
		ENodeDirection::North,
		ENodeDirection::South,
		ENodeDirection::East,
		ENodeDirection::West
	};

	ENodeDirection DirectionToTarget = ENodeDirection::None;
	bool bFoundDirection = false;

	for (ENodeDirection Direction : AllDirections)
	{
		UHGONodeGraphComponent* NodeInDirection = CurrentNode->GetNodeInDirection(Direction);
		if (NodeInDirection && NodeInDirection->NodeData.NodeID == TargetNodeID)
		{
			DirectionToTarget = Direction;
			bFoundDirection = true;
			break;
		}
	}

	if (!bFoundDirection)
	{
		UE_LOG(LogTemp, Error, TEXT("[Movement] Target node %d is not adjacent to current node %d"), 
			TargetNodeID, CurrentNode->NodeData.NodeID);
		return false;
	}

	// Use the existing TryMoveInDirection method
	return TryMoveInDirection(DirectionToTarget);
}

void UHGOGraphMovementComponent::SetCurrentNode(UHGONodeGraphComponent* NewNode)
{
	if (NewNode)
	{
		CurrentNode = NewNode;
		GetOwner()->SetActorLocation(NewNode->GetComponentLocation());
		bIsMoving = false;
		MovementProgress = 0.0f;
	}
}

bool UHGOGraphMovementComponent::IsNodeAdjacent(UHGONodeGraphComponent* TargetedNode) const
{
	if (!CurrentNode || !TargetedNode)
	{
		return false;
	}

	// Directions possibles
	TArray<ENodeDirection> AllDirections = {
		ENodeDirection::North,
		ENodeDirection::South,
		ENodeDirection::East,
		ENodeDirection::West
	};

	for (ENodeDirection Direction : AllDirections)
	{
		UHGONodeGraphComponent* AdjacentNode = CurrentNode->GetNodeInDirection(Direction);

		if (AdjacentNode && AdjacentNode == TargetedNode)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.f,
				FColor::Green,
				FString::Printf(TEXT("[Graph] ✓ Node %d is adjacent (Direction: %s)"),
					TargetedNode->NodeData.NodeID,
					*UEnum::GetValueAsString(Direction))
			);

			return true; // La node est adjacente (peu importe la direction)
		}
	}

	GEngine->AddOnScreenDebugMessage(
		-1,
		2.f,
		FColor::Red,
		TEXT("[Graph] Node not adjacent")
	);

	return false;
}

bool UHGOGraphMovementComponent::IsNodeInAlignedDirection(UHGONodeGraphComponent* TargetedNode, ENodeDirection& OutDirection) const
{
	if (!CurrentNode || !TargetedNode)
	{
		OutDirection = ENodeDirection::None;
		return false;
	}

	// Trouver le générateur pour parcourir le graphe
	AHGOTacticalLevelGenerator* Generator = nullptr;
	for (TActorIterator<AHGOTacticalLevelGenerator> GeneratorItr(GetWorld()); GeneratorItr; ++GeneratorItr)
	{
		Generator = *GeneratorItr;
		break;
	}

	if (!Generator)
	{
		OutDirection = ENodeDirection::None;
		return false;
	}

	// Tester chaque direction cardinale
	TArray<ENodeDirection> AllDirections = {
		ENodeDirection::North,
		ENodeDirection::South,
		ENodeDirection::East,
		ENodeDirection::West
	};

	for (ENodeDirection Direction : AllDirections)
	{
		// Parcourir dans cette direction jusqu'à trouver la target ou un dead-end
		UHGONodeGraphComponent* CurrentCheck = CurrentNode;
		bool bFoundPath = false;

		// Parcourir jusqu'à 20 nodes max pour éviter les boucles infinies
		for (int32 i = 0; i < 20; ++i)
		{
			UHGONodeGraphComponent* NextNode = CurrentCheck->GetNodeInDirection(Direction);
			
			if (!NextNode)
			{
				// Dead-end, pas de connexion dans cette direction
				break;
			}

			if (NextNode->NodeData.NodeID == TargetedNode->NodeData.NodeID)
			{
				// Trouvé la cible !
				bFoundPath = true;
				break;
			}

			// Continuer dans la même direction
			CurrentCheck = NextNode;
		}

		if (bFoundPath)
		{
			OutDirection = Direction;
			return true;
		}
	}

	// Aucun alignement trouvé
	OutDirection = ENodeDirection::None;
	return false;
}

void UHGOGraphMovementComponent::SwitchWorldGraph()
{
	UE_LOG(LogTemp, Log, TEXT("[WorldSwitch] Player triggered world switch"));

	// Trouver le générateur
	AHGOTacticalLevelGenerator* Generator = nullptr;
	for (TActorIterator<AHGOTacticalLevelGenerator> GeneratorItr(GetWorld()); GeneratorItr; ++GeneratorItr)
	{
		Generator = *GeneratorItr;
		break;
	}

	if (!Generator)
		return;
	
	int32 LinkedNodeID = CurrentNode->NodeData.LinkedUpsideDownNodeID;

	if (LinkedNodeID < 0)
		return;
	
	UHGONodeGraphComponent* LinkedNode = nullptr;

	for (UHGONodeGraphComponent* NodeComp : Generator->NodeGraphs)
	{
		if (!NodeComp) continue;
    	
		if (NodeComp->NodeData.LinkedUpsideDownNodeID == LinkedNodeID)
		{
			if (!bInUpsideDownWorld && NodeComp->NodeData.bIsUpsideDownNode)
			{
				LinkedNode = NodeComp;
				break;
			}
			else if (bInUpsideDownWorld && !NodeComp->NodeData.bIsUpsideDownNode)
			{
				LinkedNode = NodeComp;
				break;
			}
		}
	}

	if (!LinkedNode)
	{
		UE_LOG(LogTemp, Error, TEXT("Linked node not found! ID: %d"), LinkedNodeID);
		return;
	}

	// Bind au delegate de fin de séquence
	if (!Generator->OnSwitchWorldAnimCompleted.IsAlreadyBound(this, &UHGOGraphMovementComponent::OnWorldSwitchAnimationComplete))
	{
		Generator->OnSwitchWorldAnimCompleted.AddDynamic(this, &UHGOGraphMovementComponent::OnWorldSwitchAnimationComplete);
	}

	// Sauvegarder la node cible pour après l'animation
	CachedLinkedNode = LinkedNode;

	// Démarrer la séquence d'animation
	Generator->StartWorldSwitchSequence();

	// Le reste (broadcast OnSwitchWorldGraph, téléportation) se fera dans OnWorldSwitchAnimationComplete
	if (bInUpsideDownWorld)
	{
		UE_LOG(LogTemp, Log, TEXT("Switching to NORMAL world"));
		GetWorld()->GetAuthGameMode<AHGOGameMode>()->OnSwitchWorldGraph.Broadcast(false);
		bInUpsideDownWorld = false;
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Switching to UPSIDE-DOWN world"));
		GetWorld()->GetAuthGameMode<AHGOGameMode>()->OnSwitchWorldGraph.Broadcast(true);
		bInUpsideDownWorld = true;
	}
}

void UHGOGraphMovementComponent::OnWorldSwitchAnimationComplete()
{
	UE_LOG(LogTemp, Log, TEXT("[WorldSwitch] Animation complete, performing world switch"));

	if (!CachedLinkedNode)
	{
		UE_LOG(LogTemp, Error, TEXT("[WorldSwitch] No cached linked node!"));
		return;
	}

	// Téléporter le joueur
	CurrentNode = CachedLinkedNode;
	GetOwner()->SetActorLocation(CachedLinkedNode->GetComponentLocation());

	UE_LOG(LogTemp, Log, TEXT("Switched to node ID: %d"), CachedLinkedNode->NodeData.NodeID);

	// Garder le broadcast au cas où quelque chose l'écoute
	OnMovementCompleted.Broadcast(CurrentNode->NodeData.NodeID);

	// Clear le cache
	CachedLinkedNode = nullptr;

	// Vérifier si un ennemi peut voir le joueur dans le nouveau monde
	if (AHGOPlayerPawn* Player = Cast<AHGOPlayerPawn>(GetOwner()))
	{
		for (TActorIterator<AHGOEnemyPawn> EnemyItr(GetWorld()); EnemyItr; ++EnemyItr)
		{
			AHGOEnemyPawn* Enemy = *EnemyItr;
			if (Enemy && Enemy->CheckAndKillPlayer())
			{
				break;
			}
		}
	}

	// L'animation réelle est finie.
	// Le passage au tour ennemi ne se fera que quand le délai mini sera aussi écoulé.
	bWorldSwitchAnimationCompleted = true;
	TryCompletePendingWorldSwitch();
}

void UHGOGraphMovementComponent::UpdateMovement(float DeltaTime)
{
	if (!bIsMoving || !CurrentNode || !TargetNode)
		return;
	
	MovementProgress += DeltaTime * MovementSpeed / 100.0f;

	if (MovementProgress >= 1.0f)
	{
		// Movement complete - mettre le pawn à la position finale
		GetOwner()->SetActorLocation(TargetNode->GetComponentLocation());
		bIsMoving = false;
		MovementProgress = 0.0f;

		UE_LOG(LogTemp, Log, TEXT("[Movement] Arrived at node %d"), TargetNode->NodeData.NodeID);

		// Notifier la fin du mouvement
		NotifyMovementCompleted();
	}
	else
	{
		// Interpolate position
		FVector StartPos = CurrentNode->GetComponentLocation();
		FVector EndPos = TargetNode->GetComponentLocation();
		FVector NewPos = FMath::Lerp(StartPos, EndPos, MovementProgress);
		
		float ArcHeight = 5.0f;
			
		// Calculer la hauteur en utilisant une courbe parabolique
		float HeightOffset = FMath::Sin(MovementProgress * PI) * ArcHeight;
			
		NewPos.Z += HeightOffset;
        
		GetOwner()->SetActorLocation(NewPos);
	}
}

void UHGOGraphMovementComponent::UpdateGrabFeedback(float DeltaTime)
{
	// Empty for now - could add visual feedback here
}

void UHGOGraphMovementComponent::HideShowGraph(TArray<UHGOEdgeGraphComponent*> EdgesToProcess, TArray<UHGONodeGraphComponent*> NodesToProcess, bool bHide)
{
	for (UHGOEdgeGraphComponent* EdgeComp : EdgesToProcess)
	{
		if (EdgeComp)
		{
			EdgeComp->SetVisibility(!bHide, true);
		}
	}
    
	for (UHGONodeGraphComponent* NodeComp : NodesToProcess)
	{
		if (NodeComp)
		{
			NodeComp->SetVisibility(!bHide, true);
		}
	}
}

void UHGOGraphMovementComponent::NotifyMovementStarted()
{
	if (UWorld* World = GetWorld())
	{
		if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
		{
			TurnManager->RegisterActionStarted();
		}
	}
}

void UHGOGraphMovementComponent::NotifyMovementCompleted()
{
	// IMPORTANT: Toujours mettre à jour CurrentNode après le mouvement
	if (TargetNode)
	{
		CurrentNode = TargetNode;
		TargetNode = nullptr;

		OnMovementCompleted.Broadcast(CurrentNode->NodeData.NodeID);
	}

	// CAS 1: C'est un ENNEMI qui vient de bouger
	if (AHGOEnemyPawn* EnemyPawn = Cast<AHGOEnemyPawn>(GetOwner()))
	{
		EnemyPawn->CheckAndKillPlayer();

		// SOUS-CAS 1A: Ennemi en train d'être poussé
		if (EnemyPawn->bBeingPushed)
		{
			// Trouver l'index de la node actuelle dans le path de push
			int32 CurrentIndexInPush = EnemyPawn->PushPathNodeIDs.Find(CurrentNode->NodeData.NodeID);
			
			if (CurrentIndexInPush != INDEX_NONE && CurrentIndexInPush < EnemyPawn->PushPathNodeIDs.Num() - 1)
			{
				// Encore des nodes à parcourir
				int32 NextNodeID = EnemyPawn->PushPathNodeIDs[CurrentIndexInPush + 1];
				
				GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Magenta,
					FString::Printf(TEXT("[Push] Continuing push to node %d (%d/%d)"), 
						NextNodeID, CurrentIndexInPush + 1, EnemyPawn->PushPathNodeIDs.Num() - 1));
				
				if (TryMoveToNodeID(NextNodeID))
				{
					// Le mouvement continue, on ne termine pas le tour
					return;
				}
			}
			else
			{
				// Push terminé !
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
					TEXT("[Push] ★ PUSH COMPLETE ★"));
				
				EnemyPawn->bBeingPushed = false;
				
				// Vérifier si on est sur la patrol
				if (!EnemyPawn->IsNodeInPatrol(CurrentNode->NodeData.NodeID))
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
						TEXT("[Push] Enemy out of patrol - will return next turn"));
					
					EnemyPawn->StartReturnToPatrol();
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
						TEXT("[Push] Enemy still on patrol - continuing normally"));
				}
				
				// Push terminé, on ne change pas de tour (c'est toujours au joueur)
				return;
			}
		}
		
		// Pas de kill, rotation normale
		// SAUF si l'ennemi vient d'arriver sur un portail : construire le portail ce même tour
		if (CurrentNode->NodeData.NodeType == ENodeType::EnemyPortal)
		{
			// Arrivée sur le portail : construction immédiate, pas de rotation
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Purple,
				TEXT("[Enemy] Arrived at portal - building immediately this turn"));
			EnemyPawn->HandleEnemyPortal();
		}
		else
		{
			EnemyPawn->ExecuteEnemyRotation();

			// Vérifier si l'ennemi est adjacent au joueur (même monde uniquement)
			EnemyPawn->CheckAndKillPlayer();
		}

		return;	
	}
	
	// CAS 2: C'est le JOUEUR qui vient de bouger
	if (AHGOPlayerPawn* Player = Cast<AHGOPlayerPawn>(GetOwner()))
	{
		if (CurrentNode->NodeData.NodeType != ENodeType::PlayerPortal)
		{
			bSwitchLastRound = false; // Réinitialiser le flag si on n'est pas sur un portail
		}
		
		// PRIORITÉ 1: Vérifier si le joueur a atteint le Goal
		if (CurrentNode->NodeData.NodeType == ENodeType::Goal)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Movement] Player reached GOAL! Level complete!"));
			
			Player->CompleteLevel();
			
			// Terminer le tour
			if (UWorld* World = GetWorld())
			{
				if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
				{
					TurnManager->RegisterActionCompleted();
				}
			}
			return;
		}
		
		// PRIORITÉ 2: Vérifier si un ennemi peut voir le joueur
		for (TActorIterator<AHGOEnemyPawn> EnemyItr(GetWorld()); EnemyItr; ++EnemyItr)
		{
			AHGOEnemyPawn* Enemy = *EnemyItr;
			if (Enemy && Enemy->CheckAndKillPlayer())
			{
				// Le joueur a été tué par cet ennemi, terminer le tour
				if (UWorld* World = GetWorld())
				{
					if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
					{
						TurnManager->RegisterActionCompleted();
					}
				}
				return;
			}
		}

		if (CurrentNode->NodeData.NodeType == ENodeType::PlayerPortal && !bSwitchLastRound)
		{
			UE_LOG(LogTemp, Log, TEXT("[Movement] Player reached PlayerPortal node!"));

			bSwitchLastRound = true;

			// On garde le tour joueur "ouvert" pendant toute la transition
			bWaitingForWorldSwitchCompletion = true;
			bWorldSwitchAnimationCompleted = false;
			WorldSwitchDelayRemaining = WorldSwitchTurnDelay;

			GEngine->AddOnScreenDebugMessage(
				1,
				5.0f,
				FColor::Emerald,
				FString::Printf(TEXT("World switch delay started: %.2fs"), WorldSwitchTurnDelay),
				false
			);

			SwitchWorldGraph();

			// IMPORTANT : ne pas finir l'action maintenant
			return;
		}
	}
	
	// Aucun événement spécial, compléter l'action normalement
	if (UWorld* World = GetWorld())
	{
		if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
		{
			TurnManager->RegisterActionCompleted();
		}
	}
}

// Called when the game starts
void UHGOGraphMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void UHGOGraphMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsMoving)
	{
		UpdateMovement(DeltaTime);
	}

	UpdateWorldSwitchWait(DeltaTime);

	if (AHGOPlayerPawn* Player = Cast<AHGOPlayerPawn>(GetOwner()))
	{
		GEngine->AddOnScreenDebugMessage(
			1,
			0.0f,
			FColor::Cyan,
			FString::Printf(TEXT("bSwitchLastRound: %s"), bSwitchLastRound ? TEXT("TRUE") : TEXT("FALSE"))
		);
	}
}

void UHGOGraphMovementComponent::UpdateWorldSwitchWait(float DeltaTime)
{
	if (!bWaitingForWorldSwitchCompletion)
	{
		return;
	}

	if (WorldSwitchDelayRemaining > 0.0f)
	{
		WorldSwitchDelayRemaining -= DeltaTime;
	}

	TryCompletePendingWorldSwitch();
}

void UHGOGraphMovementComponent::TryCompletePendingWorldSwitch()
{
	if (!bWaitingForWorldSwitchCompletion)
	{
		return;
	}

	// On attend :
	// 1) la vraie fin de l'anim/séquence
	// 2) le délai mini configuré
	if (!bWorldSwitchAnimationCompleted)
	{
		return;
	}

	if (WorldSwitchDelayRemaining > 0.0f)
	{
		return;
	}

	bWaitingForWorldSwitchCompletion = false;
	bWorldSwitchAnimationCompleted = false;
	WorldSwitchDelayRemaining = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("[WorldSwitch] Transition fully completed, ending player action"));

	if (UWorld* World = GetWorld())
	{
		if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
		{
			TurnManager->RegisterActionCompleted();
		}
	}
}