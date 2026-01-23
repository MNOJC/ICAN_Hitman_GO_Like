// Fill out your copyright notice in the Description page of Project Settings.


#include "Graph/HGOTacticalLevelGenerator.h"

// Sets default values
AHGOTacticalLevelGenerator::AHGOTacticalLevelGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AHGOTacticalLevelGenerator::GenerateVisualGraph()
{
    if (!LevelData)
    {
        UE_LOG(LogTemp, Warning, TEXT("LevelData is null, cannot generate visual graph"));
        return;
    }
    
    if (!NodeGraphClass || !EdgeGraphClass)
    {
        UE_LOG(LogTemp, Error, TEXT("NodeGraphClass or EdgeGraphClass is not assigned!"));
        return;
    }
    
    ClearVisualGraph();
    
    TMap<int32, UHGONodeGraphComponent*> SpawnedNodeMap;
    
    for (const FNodeData& NodeData : LevelData->Nodes)
    {
        UHGONodeGraphComponent* NodeComp = NewObject<UHGONodeGraphComponent>(this, NodeGraphClass);

        if (!NodeComp)
            continue;

        NodeComp->SetupAttachment(GetRootComponent());
        NodeComp->RegisterComponent();
        
        NodeComp->SetWorldLocation(NodeData.Position);

        NodeComp->NodeData = NodeData;

        if (NodeData.bIsUpsideDownNode)
            NodeComp->SetVisibility(false, true);
           
        

        NodeGraphs.Add(NodeComp);
        SpawnedNodeMap.Add(NodeData.NodeID, NodeComp);
    }
    
    for (const FEdgeData& EdgeData : LevelData->Edges)
    {
        UHGONodeGraphComponent** SourceNodePtr = SpawnedNodeMap.Find(EdgeData.SourceNodeID);
        UHGONodeGraphComponent** TargetNodePtr = SpawnedNodeMap.Find(EdgeData.TargetNodeID);

        if (!SourceNodePtr || !TargetNodePtr)
            continue;

        UHGONodeGraphComponent* SourceNode = *SourceNodePtr;
        UHGONodeGraphComponent* TargetNode = *TargetNodePtr;

        SourceNode->ConnectedNodes.Add(EdgeData.Direction, TargetNode);

        if (EdgeData.bIsBidirectional)
        {
            ENodeDirection Opposite = GetOppositeDirection(EdgeData.Direction);
            TargetNode->ConnectedNodes.Add(Opposite, SourceNode);
        }

        FVector SourcePos = SourceNode->GetComponentLocation();
        FVector TargetPos = TargetNode->GetComponentLocation();

        FVector MidPoint = (SourcePos + TargetPos) * 0.5f;
        FVector Direction = TargetPos - SourcePos;
        FRotator EdgeRotation = Direction.Rotation();
        float Distance = Direction.Size();

        UHGOEdgeGraphComponent* EdgeComp = NewObject<UHGOEdgeGraphComponent>(this, EdgeGraphClass);

        if (!EdgeComp)
            continue;

        EdgeComp->SetupAttachment(GetRootComponent());
        EdgeComp->RegisterComponent();

        EdgeComp->SetWorldLocation(MidPoint);
        EdgeComp->SetWorldRotation(EdgeRotation);

        EdgeComp->EdgeData = EdgeData;

        FVector EdgeScale(Distance / 100.f, .1f, 1.f);
        EdgeComp->SetWorldScale3D(EdgeScale);

        bool bIsUpsideDownEdge = SourceNode->NodeData.bIsUpsideDownNode || TargetNode->NodeData.bIsUpsideDownNode;
        
        if (bIsUpsideDownEdge)
            EdgeComp->SetVisibility(false, true);
            
        

        EdgeGraphs.Add(EdgeComp);
        
    }

}


void AHGOTacticalLevelGenerator::ClearVisualGraph()
{
    for (UHGONodeGraphComponent* Node : NodeGraphs)
    {
        if (Node)
        {
            Node->DestroyComponent();
        }
    }
    NodeGraphs.Empty();
    
    for (UHGOEdgeGraphComponent* Edge : EdgeGraphs)
    {
        if (Edge)
        {
            Edge->DestroyComponent();
        }
    }
    EdgeGraphs.Empty();
}

// Called when the game starts or when spawned
void AHGOTacticalLevelGenerator::BeginPlay()
{
	Super::BeginPlay();

	GenerateVisualGraph();
}

// Called every frame
void AHGOTacticalLevelGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

ENodeDirection AHGOTacticalLevelGenerator::GetOppositeDirection(ENodeDirection Direction)
{
    switch (Direction)
    {
    case ENodeDirection::North: return ENodeDirection::South;
    case ENodeDirection::South: return ENodeDirection::North;
    case ENodeDirection::East:  return ENodeDirection::West;
    case ENodeDirection::West:  return ENodeDirection::East;
    default: return ENodeDirection::None;
    }
}