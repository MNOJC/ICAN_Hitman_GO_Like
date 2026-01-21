// Fill out your copyright notice in the Description page of Project Settings.

#include "HGONodeEditor.h"
#include "EngineUtils.h"

AHGONodeEditor::AHGONodeEditor()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    NodeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeMesh"));
    NodeMesh->SetupAttachment(Root);
}

void AHGONodeEditor::Destroyed()
{
    DeleteConnectedEdges();
    Super::Destroyed();
}

void AHGONodeEditor::DeleteConnectedEdges()
{
    int32 DeletedEdgesCount = 0;
    TArray<AHGOEdgeEditor*> EdgesToDelete;

    // Parcourir tous les edges du niveau
    for (TActorIterator<AHGOEdgeEditor> It(GetWorld()); It; ++It)
    {
        AHGOEdgeEditor* Edge = *It;
        
        if (Edge)
        {
            // Vérifier si cet edge est connecté à ce node (source ou target)
            if (Edge->EdgeData.SourceNodeID == NodeData.NodeID || 
                Edge->EdgeData.TargetNodeID == NodeData.NodeID)
            {
                EdgesToDelete.Add(Edge);
            }
        }
    }

    // Supprimer les edges trouvés
    for (AHGOEdgeEditor* EdgeToDelete : EdgesToDelete)
    {
        if (EdgeToDelete)
        {
            UE_LOG(LogTemp, Log, TEXT("Deleting Edge %d (connected to Node %d)"), 
                EdgeToDelete->EdgeData.EdgeID, 
                NodeData.NodeID);
            
            EdgeToDelete->Destroy();
            DeletedEdgesCount++;
        }
    }

    if (DeletedEdgesCount > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Node %d deleted with %d connected edge(s)"), 
            NodeData.NodeID, 
            DeletedEdgesCount);
    }
}

void AHGONodeEditor::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);

    if (!bFinished)
        return;
        
    NodeData.Position = GetActorLocation();
}

void AHGONodeEditor::BeginPlay()
{
    Super::BeginPlay();
}

void AHGONodeEditor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
