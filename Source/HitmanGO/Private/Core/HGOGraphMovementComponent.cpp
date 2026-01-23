// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/HGOGraphMovementComponent.h"
#include "EngineUtils.h"
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
	if (bIsMoving)
	{
		return false;
	}
	
	if (!CurrentNode)
	{
		return false;
	}
	
	UHGONodeGraphComponent* NextNode = CurrentNode->GetNodeInDirection(Direction);

	if (!NextNode)
	{
		return false;
	}
	
	TargetNode = NextNode;
	bIsMoving = true;
	MovementProgress = 0.0f;

	return true;
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
		GetOwner()->SetActorLocation(TargetNode->GetComponentLocation());
		CurrentNode = TargetNode;
		TargetNode = nullptr;
		bIsMoving = false;
		MovementProgress = 0.0f;

		if (CurrentNode->NodeData.NodeType == ENodeType::UpsideDownPortal)
		{
			UE_LOG(LogTemp, Log, TEXT("Reached UpsideDown portal node!"));
			SwitchWorldGraph();
		}
	}
	else
	{
		FVector StartPos = CurrentNode->GetComponentLocation();
		FVector EndPos = TargetNode->GetComponentLocation();
		FVector NewPos = FMath::Lerp(StartPos, EndPos, MovementProgress);
        
		GetOwner()->SetActorLocation(NewPos);
	}
}

void UHGOGraphMovementComponent::UpdateGrabFeedback(float DeltaTime)
{
	
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

