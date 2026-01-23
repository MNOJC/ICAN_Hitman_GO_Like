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
    TArray<AHGOEdgeEditor*> EdgesToDelete;
    
    for (TActorIterator<AHGOEdgeEditor> It(GetWorld()); It; ++It)
    {
        AHGOEdgeEditor* Edge = *It;
        
        if (Edge)
        {
            if (Edge->EdgeData.SourceNodeID == NodeData.NodeID || Edge->EdgeData.TargetNodeID == NodeData.NodeID)
            {
                EdgesToDelete.Add(Edge);
            }
        }
    }
    
    for (AHGOEdgeEditor* EdgeToDelete : EdgesToDelete)
    {
        if (EdgeToDelete)
        {
            EdgeToDelete->Destroy();
        }
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
