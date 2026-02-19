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
}

void UHGOGraphMovementComponent::UpdateMovement(float DeltaTime)
{
	if (!bIsMoving || !CurrentNode || !TargetNode)
		return;
	
	MovementProgress += DeltaTime * MovementSpeed / 100.0f;

	if (MovementProgress >= 1.0f)
	{
		// Movement complete
		GetOwner()->SetActorLocation(TargetNode->GetComponentLocation());
		CurrentNode = TargetNode;
		TargetNode = nullptr;
		bIsMoving = false;
		MovementProgress = 0.0f;

		UE_LOG(LogTemp, Log, TEXT("[Movement] Arrived at node %d"), CurrentNode->NodeData.NodeID);

		// Check for PlayerPortal BEFORE notifying turn system
		if (CurrentNode->NodeData.NodeType == ENodeType::PlayerPortal)
		{
			UE_LOG(LogTemp, Log, TEXT("[Movement] Player reached PlayerPortal node!"));
			SwitchWorldGraph();
		} 
		else
		{
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
	if(AHGOEnemyPawn* EnemyPawn = Cast<AHGOEnemyPawn>(GetOwner()))
	{
		EnemyPawn->ExecuteEnemyRotation();
		return;	
	}
	
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
