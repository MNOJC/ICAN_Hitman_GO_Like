// Fill out your copyright notice in the Description page of Project Settings.

#include "HGOGraphManagerEditor.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "Selection.h"

AHGOGraphManagerEditor::AHGOGraphManagerEditor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AHGOGraphManagerEditor::BeginPlay()
{
    Super::BeginPlay();
}

void AHGOGraphManagerEditor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AHGOGraphManagerEditor::CreateConnectionFromSelection()
{
    TArray<AHGONodeEditor*> SelectedNodes = GetSelectedNodesInEditor();
    
    if (SelectedNodes.Num() != 2)
        return;
    
    AHGONodeEditor* FirstNode = SelectedNodes[0];
    AHGONodeEditor* SecondNode = SelectedNodes[1];

    CreateEdgeBetweenNodes(FirstNode, SecondNode);
}

AHGONodeEditor* AHGOGraphManagerEditor::CreateNewNode(FVector SpawnLocation)
{
    if (!NodeClass)
    {
        UE_LOG(LogTemp, Error, TEXT("NodeClass is not set! Please assign it in the Graph Manager."));
        
        #if WITH_EDITOR
        FMessageDialog::Open(EAppMsgType::Ok, 
            FText::FromString(TEXT("NodeClass is not assigned in the Graph Manager!")));
        #endif
        
        return nullptr;
    }
    
    if (SpawnLocation.IsZero())
    {
        #if WITH_EDITOR
        if (GEditor && GEditor->GetActiveViewport())
        {
            FViewport* Viewport = GEditor->GetActiveViewport();
            FEditorViewportClient* ViewportClient = (FEditorViewportClient*)Viewport->GetClient();
            
            if (ViewportClient)
            {
                FVector CameraLocation = ViewportClient->GetViewLocation();
                FRotator CameraRotation = ViewportClient->GetViewRotation();

                SpawnLocation = CameraLocation + CameraRotation.Vector() * 500.0f;
            }
        }
        #endif
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AHGONodeEditor* NewNode = GetWorld()->SpawnActor<AHGONodeEditor>(
        NodeClass,
        SpawnLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (NewNode)
    {
        int32 NewNodeID = GetNextNodeID();
        NewNode->NodeData.NodeID = NewNodeID;
        NewNode->NodeData.Position = SpawnLocation;
        NewNode->NodeData.NodeType = ENodeType::Normal; // Par défaut

        #if WITH_EDITOR
        NewNode->SetActorLabel(FString::Printf(TEXT("Node_%d"), NewNodeID));
        
        // Sélectionner le nouveau node dans l'éditeur
        if (GEditor)
        {
            GEditor->SelectNone(true, true);
            GEditor->SelectActor(NewNode, true, true);
        }
        #endif

        UE_LOG(LogTemp, Log, TEXT("Node created with ID: %d at location: %s"), 
            NewNodeID, *SpawnLocation.ToString());

        #if WITH_EDITOR
        // Message de confirmation
        FText SuccessMessage = FText::FromString(FString::Printf(
            TEXT("Node created!\nID: %d"), NewNodeID));
        FMessageDialog::Open(EAppMsgType::Ok, SuccessMessage);
        #endif

        return NewNode;
    }

    return nullptr;
}

void AHGOGraphManagerEditor::SaveGraphDataAsset(UHGOTacticalLevelData* GraphDataAsset)
{
    if (!GraphDataAsset)
        return;
    
    GraphDataAsset->Nodes.Empty();
    GraphDataAsset->Edges.Empty();

    TActorIterator<AHGONodeEditor> NodeIt(GetWorld());
    for (; NodeIt; ++NodeIt)
    {
        AHGONodeEditor* Node = *NodeIt;
        if (Node)
        {
            GraphDataAsset->Nodes.Add(Node->NodeData);
        }
    }

    TActorIterator<AHGOEdgeEditor> EdgeIt(GetWorld());
    for (; EdgeIt; ++EdgeIt)
    {
        AHGOEdgeEditor* Edge = *EdgeIt;
        if (Edge)
        {
            GraphDataAsset->Edges.Add(Edge->EdgeData);
        }
    }

    
}

TArray<AHGONodeEditor*> AHGOGraphManagerEditor::GetSelectedNodesInEditor()
{
    TArray<AHGONodeEditor*> SelectedNodes;
    
    if (GEditor)
    {
        USelection* SelectedActors = GEditor->GetSelectedActors();
        
        if (SelectedActors)
        {
            for (FSelectionIterator It(*SelectedActors); It; ++It)
            {
                AActor* Actor = Cast<AActor>(*It);
                if (Actor)
                {
                    AHGONodeEditor* NodeEditor = Cast<AHGONodeEditor>(Actor);
                    if (NodeEditor)
                    {
                        SelectedNodes.Add(NodeEditor);
                    }
                }
            }
        }
    }

    return SelectedNodes;
}

void AHGOGraphManagerEditor::CreateEdgeBetweenNodes(AHGONodeEditor* Source, AHGONodeEditor* Target)
{
    if (!Source || !Target)
        return;
    
    if (!EdgeClass)
        return;
    
    FVector SourcePos = Source->GetActorLocation();
    FVector TargetPos = Target->GetActorLocation();
    FVector MidPoint = (SourcePos + TargetPos) / 2.0f;
    FVector Direction = TargetPos - SourcePos;
    FRotator EdgeRotation = Direction.Rotation();
    float Distance = FVector::Dist(SourcePos, TargetPos);
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AHGOEdgeEditor* NewEdge = GetWorld()->SpawnActor<AHGOEdgeEditor>(EdgeClass, MidPoint, EdgeRotation, SpawnParams);

    if (NewEdge)
    {
        NewEdge->EdgeData.EdgeID = GetNextEdgeID();
        NewEdge->EdgeData.SourceNodeID = Source->NodeData.NodeID;
        NewEdge->EdgeData.TargetNodeID = Target->NodeData.NodeID;
        NewEdge->EdgeData.bIsBidirectional = true;
        NewEdge->EdgeData.Direction = CalculateDirection(SourcePos, TargetPos);
        
        FVector EdgeScale = FVector(Distance / 100.0f, 1.0f, 1.0f);
        NewEdge->SetActorScale3D(EdgeScale);
        
    }
}

int32 AHGOGraphManagerEditor::GetNextEdgeID()
{
    int32 MaxID = 0;
    
    for (TActorIterator<AHGOEdgeEditor> It(GetWorld()); It; ++It)
    {
        AHGOEdgeEditor* Edge = *It;
        if (Edge && Edge->EdgeData.EdgeID > MaxID)
        {
            MaxID = Edge->EdgeData.EdgeID;
        }
    }
    
    return MaxID + 1;
}

int32 AHGOGraphManagerEditor::GetNextNodeID()
{
    int32 MaxID = 0;
    
    for (TActorIterator<AHGONodeEditor> It(GetWorld()); It; ++It)
    {
        AHGONodeEditor* Node = *It;
        if (Node && Node->NodeData.NodeID > MaxID)
        {
            MaxID = Node->NodeData.NodeID;
        }
    }
    
    return MaxID + 1;
}

ENodeDirection AHGOGraphManagerEditor::CalculateDirection(FVector SourcePos, FVector TargetPos)
{
    FVector Direction = (TargetPos - SourcePos).GetSafeNormal();
    
    float Angle = FMath::Atan2(Direction.X, Direction.Y) * (180.0f / PI);

    if (Angle < 0)
        Angle += 360.0f;
    
    if (Angle >= 45.0f && Angle < 135.0f)
    {
        return ENodeDirection::North; 
    }
    else if (Angle >= 135.0f && Angle < 225.0f)
    {
        return ENodeDirection::West; 
    }
    else if (Angle >= 225.0f && Angle < 315.0f)
    {
        return ENodeDirection::South; 
    }
    else 
    {
        return ENodeDirection::East;
    }
}
