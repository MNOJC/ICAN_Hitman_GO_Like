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
    
    if (!NodeGraphClass)
    {
        UE_LOG(LogTemp, Error, TEXT("NodeGraphClass is not assigned!"));
        return;
    }

    if (!EdgeGraphClass)
    {
        UE_LOG(LogTemp, Error, TEXT("EdgeGraphClass is not assigned!"));
        return;
    }
    
    ClearVisualGraph();
    
    TMap<int32, AHGONodeGraph*> SpawnedNodeMap;
    
    for (const FNodeData& NodeData : LevelData->Nodes)
    {
        FVector SpawnLocation = GetActorLocation() + NodeData.Position;
        FRotator SpawnRotation = FRotator::ZeroRotator;
        
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
        AHGONodeGraph* NodeActor = GetWorld()->SpawnActor<AHGONodeGraph>(
            NodeGraphClass,
            SpawnLocation,
            SpawnRotation,
            SpawnParams
        );

        if (NodeActor)
        {
            NodeActor->NodeData = NodeData;
            
            NodeGraphs.Add(NodeActor);
            SpawnedNodeMap.Add(NodeData.NodeID, NodeActor);

            #if WITH_EDITOR
            NodeActor->SetActorLabel(FString::Printf(TEXT("Node_%d"), NodeData.NodeID));
            #endif
        }
    }
    
    for (const FEdgeData& EdgeData : LevelData->Edges)
    {
        AHGONodeGraph** SourceNodePtr = SpawnedNodeMap.Find(EdgeData.SourceNodeID);
        AHGONodeGraph** TargetNodePtr = SpawnedNodeMap.Find(EdgeData.TargetNodeID);
        
        if (!SourceNodePtr || !TargetNodePtr)
        {
            UE_LOG(LogTemp, Warning, TEXT("Edge %d: Source or Target node not found"), 
                EdgeData.EdgeID);
            continue;
        }

        AHGONodeGraph* SourceNode = *SourceNodePtr;
        AHGONodeGraph* TargetNode = *TargetNodePtr;
        
        FVector SourcePos = SourceNode->GetActorLocation();
        FVector TargetPos = TargetNode->GetActorLocation();
        
        FVector MidPoint = (SourcePos + TargetPos) / 2.0f;
        
        FVector Direction = TargetPos - SourcePos;
        FRotator EdgeRotation = Direction.Rotation();
        
        float Distance = FVector::Dist(SourcePos, TargetPos);
        
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
        AHGOEdgeGraph* EdgeActor = GetWorld()->SpawnActor<AHGOEdgeGraph>(
            EdgeGraphClass,
            MidPoint,
            EdgeRotation,
            SpawnParams
        );

        if (EdgeActor && EdgeActor->EdgeMeshComponent)
        {
            EdgeActor->EdgeData = EdgeData;

            FVector EdgeScale = FVector(Distance / 100.0f, 1.0f, 1.0f);
            EdgeActor->SetActorScale3D(EdgeScale);
            
            EdgeGraphs.Add(EdgeActor);

            #if WITH_EDITOR
            EdgeActor->SetActorLabel(FString::Printf(TEXT("Edge_%d_to_%d"), 
                EdgeData.SourceNodeID,
                EdgeData.TargetNodeID));
            #endif
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Visual Graph Generated: %d nodes, %d edges"), 
        NodeGraphs.Num(), EdgeGraphs.Num());
}

void AHGOTacticalLevelGenerator::ClearVisualGraph()
{
    for (AHGONodeGraph* Node : NodeGraphs)
    {
        if (Node)
        {
            Node->Destroy();
        }
    }
    NodeGraphs.Empty();
    
    for (AHGOEdgeGraph* Edge : EdgeGraphs)
    {
        if (Edge)
        {
            Edge->Destroy();
        }
    }
    EdgeGraphs.Empty();

    UE_LOG(LogTemp, Log, TEXT("Visual Graph Cleared"));
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

