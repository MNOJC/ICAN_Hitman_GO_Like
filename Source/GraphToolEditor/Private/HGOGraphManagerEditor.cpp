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
        return nullptr;
    
    
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

                float ZLocation = 0.0f;

                for (TActorIterator<AHGONodeEditor> NodeIt(GetWorld()); NodeIt; ++NodeIt)
                {
                    AHGONodeEditor* Node = *NodeIt;
                    if (Node)
                    {
                       ZLocation = Node->GetActorLocation().Z;
                       break;
                    }                        
                }
                SpawnLocation = CameraLocation + CameraRotation.Vector() * 500.0f;
                SpawnLocation = SpawnLocation.GridSnap(GridSpacing);
                SpawnLocation = FVector(SpawnLocation.X, SpawnLocation.Y, ZLocation);
                
            }
        }
        #endif
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    GetWorld()->Modify();
    
    AHGONodeEditor* NewNode = GetWorld()->SpawnActor<AHGONodeEditor>(NodeClass, SpawnLocation,FRotator::ZeroRotator, SpawnParams);

    if (NewNode)
    {
        int32 NewNodeID = GetNextNodeID();
        NewNode->NodeData.NodeID = NewNodeID;
        NewNode->NodeData.Position = SpawnLocation;
        NewNode->NodeData.NodeType = ENodeType::Normal;
        NewNode->NodeData.bIsUpsideDownNode = bEditingUpsideDownGraph;
        
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

void AHGOGraphManagerEditor::SwitchEditingWorldType(bool bIsEditingUpsideDown)
{
    bEditingUpsideDownGraph = bIsEditingUpsideDown;
    
    TMap<int32, AHGONodeEditor*> NodeMap;

    for (TActorIterator<AHGONodeEditor> NodeIt(GetWorld()); NodeIt; ++NodeIt)
    {
        AHGONodeEditor* Node = *NodeIt;
        if (!Node)
            continue;

        bool bShouldBeVisible = (Node->NodeData.bIsUpsideDownNode == bEditingUpsideDownGraph);

        Node->SetActorHiddenInGame(!bShouldBeVisible);
        Node->SetActorEnableCollision(bShouldBeVisible);
        Node->SetIsTemporarilyHiddenInEditor(!bShouldBeVisible);

        NodeMap.Add(Node->NodeData.NodeID, Node);
    }
    
    for (TActorIterator<AHGOEdgeEditor> EdgeIt(GetWorld()); EdgeIt; ++EdgeIt)
    {
        AHGOEdgeEditor* Edge = *EdgeIt;
        if (!Edge)
            continue;

        AHGONodeEditor** SourceNode = NodeMap.Find(Edge->EdgeData.SourceNodeID);
        AHGONodeEditor** TargetNode = NodeMap.Find(Edge->EdgeData.TargetNodeID);

        bool bShouldBeVisible = false;

        if (SourceNode && *SourceNode)
        {
            if ((*SourceNode)->NodeData.bIsUpsideDownNode == bEditingUpsideDownGraph)
            {
                bShouldBeVisible = true;
            }
        }

        if (TargetNode && *TargetNode)
        {
            if ((*TargetNode)->NodeData.bIsUpsideDownNode == bEditingUpsideDownGraph)
            {
                bShouldBeVisible = true;
            }
        }

        Edge->SetActorHiddenInGame(!bShouldBeVisible);
        Edge->SetActorEnableCollision(bShouldBeVisible);
        Edge->SetIsTemporarilyHiddenInEditor(!bShouldBeVisible);
    }
}

void AHGOGraphManagerEditor::OnGridSpacingChanged(float NewGridSpacing)
{
    GridSpacing = NewGridSpacing;
}

void AHGOGraphManagerEditor::OnGridRefreshed()
{
    const float PreviousGridSpacing = ComputeCurrentGridSpacing();
    
    if (!GetWorld() || PreviousGridSpacing <= KINDA_SMALL_NUMBER)
        return;

    const float ScaleRatio = GridSpacing / PreviousGridSpacing;

    GetWorld()->Modify();

    TMap<int32, AHGONodeEditor*> NodeMap;

    // --- 1. Scale des nodes ---
    for (TActorIterator<AHGONodeEditor> NodeIt(GetWorld()); NodeIt; ++NodeIt)
    {
        AHGONodeEditor* Node = *NodeIt;
        if (!Node)
            continue;

        FVector OldPos = Node->GetActorLocation();
        FVector NewPos = OldPos * ScaleRatio;

        Node->Modify();
        Node->SetActorLocation(NewPos);
        Node->NodeData.Position = NewPos;

        NodeMap.Add(Node->NodeData.NodeID, Node);
    }

    // --- 2. Refresh des edges ---
    for (TActorIterator<AHGOEdgeEditor> EdgeIt(GetWorld()); EdgeIt; ++EdgeIt)
    {
        AHGOEdgeEditor* Edge = *EdgeIt;
        if (!Edge)
            continue;

        AHGONodeEditor** SourceNode = NodeMap.Find(Edge->EdgeData.SourceNodeID);
        AHGONodeEditor** TargetNode = NodeMap.Find(Edge->EdgeData.TargetNodeID);

        if (!SourceNode || !TargetNode || !(*SourceNode) || !(*TargetNode))
            continue;

        FVector SourcePos = (*SourceNode)->GetActorLocation();
        FVector TargetPos = (*TargetNode)->GetActorLocation();

        FVector Direction = TargetPos - SourcePos;
        float Distance = Direction.Size();
        FVector MidPoint = (SourcePos + TargetPos) * 0.5f;

        Edge->Modify();
        Edge->SetActorLocation(MidPoint);
        Edge->SetActorRotation(Direction.Rotation());
        Edge->SetActorScale3D(FVector(Distance / 100.0f, 1.f, 1.f));

        Edge->EdgeData.Direction = CalculateDirection(SourcePos, TargetPos);
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
    
    GetWorld()->Modify();
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

float AHGOGraphManagerEditor::ComputeCurrentGridSpacing() const 
{
    TArray<FVector> Positions;

    for (TActorIterator<AHGONodeEditor> It(GetWorld()); It; ++It)
    {
        if (*It)
        {
            Positions.Add(It->GetActorLocation());
        }
    }

    if (Positions.Num() < 2)
        return GridSpacing; // fallback safe

    float SmallestDelta = TNumericLimits<float>::Max();

    for (int32 i = 0; i < Positions.Num(); ++i)
    {
        for (int32 j = i + 1; j < Positions.Num(); ++j)
        {
            const FVector Delta = (Positions[i] - Positions[j]).GetAbs();

            if (Delta.X > KINDA_SMALL_NUMBER)
                SmallestDelta = FMath::Min(SmallestDelta, Delta.X);

            if (Delta.Y > KINDA_SMALL_NUMBER)
                SmallestDelta = FMath::Min(SmallestDelta, Delta.Y);
        }
    }

    return (SmallestDelta == TNumericLimits<float>::Max())
           ? GridSpacing
           : SmallestDelta;
}
