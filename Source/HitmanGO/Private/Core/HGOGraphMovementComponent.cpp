// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/HGOGraphMovementComponent.h"
#include "Core/HGOTacticalTurnManager.h"
#include "EngineUtils.h"
#include "Core/HGOEnemyPawn.h"
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

bool UHGOGraphMovementComponent::IsNodeInFrontDirection(UHGONodeGraphComponent* TargetedNode) const
{
	if (!CurrentNode || !TargetedNode || !GetOwner())
	{
		return false;
	}

	// Vérifier si la target est devant (dans la direction du forward)
	FVector CurrentPos = CurrentNode->GetComponentLocation();
	FVector TargetPos = TargetedNode->GetComponentLocation();
	FVector ToTarget = (TargetPos - CurrentPos).GetSafeNormal();
	FVector Forward = GetOwner()->GetActorForwardVector();
	
	// Calculer le dot product (cosinus de l'angle)
	float DotProduct = FVector::DotProduct(Forward, ToTarget);
	
	GEngine->AddOnScreenDebugMessage(
		-1,
		2.f,
		FColor::Yellow,
		FString::Printf(TEXT("[Vision] Checking node %d -> %d | Dot: %.2f"), 
			CurrentNode->NodeData.NodeID, TargetedNode->NodeData.NodeID, DotProduct)
	);
	
	// Si dot > 0.7, l'angle est inférieur à ~45 degrés (devant)
	if (DotProduct < 0.7f)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.f,
			FColor::Orange,
			TEXT("[Vision] Target not in front (angle too wide)")
		);
		return false;
	}

	// Vérifier qu'il y a une connexion directe entre les nodes
	// C'EST LA VRAIE VÉRIFICATION DE "1 NODE DE DISTANCE"
	TArray<ENodeDirection> AllDirections = {
		ENodeDirection::North,
		ENodeDirection::South,
		ENodeDirection::East,
		ENodeDirection::West
	};

	for (ENodeDirection Direction : AllDirections)
	{
		UHGONodeGraphComponent* AdjacentNode = CurrentNode->GetNodeInDirection(Direction);
		if (AdjacentNode && AdjacentNode->NodeData.NodeID == TargetedNode->NodeData.NodeID)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.f,
				FColor::Green,
				FString::Printf(TEXT("[Vision] ✓ PLAYER DETECTED! (Direction: %s)"), 
					*UEnum::GetValueAsString(Direction))
			);
			return true; // La node cible est directement adjacente ET devant
		}
	}

	GEngine->AddOnScreenDebugMessage(
		-1,
		2.f,
		FColor::Red,
		TEXT("[Vision] No direct connection found")
	);
	
	return false; // Pas de connexion directe
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
			// Si on est dans le monde normal, chercher le node upside-down
			if (!bInUpsideDownWorld && NodeComp->NodeData.bIsUpsideDownNode)
			{
				LinkedNode = NodeComp;
				break;
			}
			// Si on est dans le monde upside-down, chercher le node normal
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

	// Préparer les listes de nodes/edges à afficher/cacher
	TArray<UHGONodeGraphComponent*> NormalNodes;
	TArray<UHGONodeGraphComponent*> UpsideDownNodes;
	TArray<UHGOEdgeGraphComponent*> NormalEdges;
	TArray<UHGOEdgeGraphComponent*> UpsideDownEdges;

	// Séparer les nodes
	for (UHGONodeGraphComponent* NodeComp : Generator->NodeGraphs)
	{
		if (!NodeComp) continue;

		if (NodeComp->NodeData.bIsUpsideDownNode)
		{
			UpsideDownNodes.Add(NodeComp);
		}
		else
		{
			NormalNodes.Add(NodeComp);
		}
	}

	// Séparer les edges
	for (UHGOEdgeGraphComponent* EdgeComp : Generator->EdgeGraphs)
	{
		if (!EdgeComp) continue;

		// Un edge est upside-down si ses deux nodes sont upside-down
		bool bSourceIsUpsideDown = false;
		bool bTargetIsUpsideDown = false;

		for (UHGONodeGraphComponent* NodeComp : Generator->NodeGraphs)
		{
			if (!NodeComp) continue;

			if (NodeComp->NodeData.NodeID == EdgeComp->EdgeData.SourceNodeID)
			{
				bSourceIsUpsideDown = NodeComp->NodeData.bIsUpsideDownNode;
			}
			if (NodeComp->NodeData.NodeID == EdgeComp->EdgeData.TargetNodeID)
			{
				bTargetIsUpsideDown = NodeComp->NodeData.bIsUpsideDownNode;
			}
		}

		// L'edge appartient au monde upside-down si au moins un node l'est
		if (bSourceIsUpsideDown || bTargetIsUpsideDown)
		{
			UpsideDownEdges.Add(EdgeComp);
		}
		else
		{
			NormalEdges.Add(EdgeComp);
		}
	}

	// Switch le monde
	if (bInUpsideDownWorld)
	{
		// Retour au monde normal
		UE_LOG(LogTemp, Log, TEXT("Switching to NORMAL world"));
		GetWorld()->GetAuthGameMode<AHGOGameMode>()->OnSwitchWorldGraph.Broadcast(false);
        
		HideShowGraph(UpsideDownEdges, UpsideDownNodes, true);  // Cacher upside-down
		HideShowGraph(NormalEdges, NormalNodes, false);         // Afficher normal
        
		bInUpsideDownWorld = false;
	}
	else
	{
		// Passage au monde upside-down
		UE_LOG(LogTemp, Log, TEXT("Switching to UPSIDE-DOWN world"));
		GetWorld()->GetAuthGameMode<AHGOGameMode>()->OnSwitchWorldGraph.Broadcast(true);
        
		HideShowGraph(NormalEdges, NormalNodes, true);          // Cacher normal
		HideShowGraph(UpsideDownEdges, UpsideDownNodes, false); // Afficher upside-down
        
		bInUpsideDownWorld = true;
	}

	// Téléporter le joueur sur le node lié
	CurrentNode = LinkedNode;
	GetOwner()->SetActorLocation(LinkedNode->GetComponentLocation());

	UE_LOG(LogTemp, Log, TEXT("Switched to node ID: %d (IsUpsideDown: %s)"), 
		LinkedNode->NodeData.NodeID,
		LinkedNode->NodeData.bIsUpsideDownNode ? TEXT("Yes") : TEXT("No"));

	NotifyMovementCompleted();
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

		// Check for PlayerPortal BEFORE notifying turn system
		if (TargetNode->NodeData.NodeType == ENodeType::PlayerPortal)
		{
			UE_LOG(LogTemp, Log, TEXT("[Movement] Player reached PlayerPortal node!"));
			
			// Mettre à jour CurrentNode AVANT le switch de monde
			CurrentNode = TargetNode;
			TargetNode = nullptr;
			
			SwitchWorldGraph();
		} 
		else
		{
			// Notifier la fin du mouvement (CurrentNode sera mis à jour dans cette fonction)
			NotifyMovementCompleted();
		}
		
	}
	else
	{
		// Interpolate position
		FVector StartPos = CurrentNode->GetComponentLocation();
		FVector EndPos = TargetNode->GetComponentLocation();
		FVector NewPos = FMath::Lerp(StartPos, EndPos, MovementProgress);
		
		// Ajouter un arc de mouvement pour l'EnemyPawn
		if (Cast<AHGOEnemyPawn>(GetOwner()))
		{
			// Hauteur de l'arc (ajustable)
			float ArcHeight = 150.0f;
			
			// Calculer la hauteur en utilisant une courbe parabolique
			// sin donne une courbe douce qui monte puis redescend
			float HeightOffset = FMath::Sin(MovementProgress * PI) * ArcHeight;
			
			NewPos.Z += HeightOffset;
		}
        
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

		if (AHGOPlayerPawn* Player = Cast<AHGOPlayerPawn>(GetOwner()))
		{
			for (TActorIterator<AHGOEnemyPawn> EnemyItr(GetWorld()); EnemyItr; ++EnemyItr)
			{
				AHGOEnemyPawn* Enemy = *EnemyItr;

				if (Enemy)
				{
					if (Enemy->GraphMovementComponent->CurrentNode)
					{
						if(CurrentNode->NodeData.NodeID == Enemy->GraphMovementComponent->CurrentNode->NodeData.NodeID)
						{
							Player->KillPlayer();
							return; // Player is dead, no need to check further
						}
					}
				
				}
			
			}
		}
	}

	// CAS 1: C'est un ENNEMI qui vient de bouger
	if(AHGOEnemyPawn* EnemyPawn = Cast<AHGOEnemyPawn>(GetOwner()))
	{
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
				// On ne fait rien, le joueur peut continuer à jouer
				return;
			}
		}
		
		// Pas de kill, rotation normale
		EnemyPawn->ExecuteEnemyRotation();
		return;	
	}
	
	// CAS 2: C'est le JOUEUR qui vient de bouger
	if (AHGOPlayerPawn* Player = Cast<AHGOPlayerPawn>(GetOwner()))
	{
		// PRIORITÉ 1: Vérifier si le joueur a atteint le Goal
		if (CurrentNode && CurrentNode->NodeData.NodeType == ENodeType::Goal)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Movement] Player reached GOAL! Level complete!"));
			
			// Compléter le niveau
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
}
