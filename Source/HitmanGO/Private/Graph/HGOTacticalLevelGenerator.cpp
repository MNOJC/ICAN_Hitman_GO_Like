// Fill out your copyright notice in the Description page of Project Settings.

#include "Graph/HGOTacticalLevelGenerator.h"
#include "Core/HGOPlayerPawn.h"
#include "EngineUtils.h"

// Sets default values
AHGOTacticalLevelGenerator::AHGOTacticalLevelGenerator()
{
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
    
    // GÉNÉRER LES NODES (CACHÉES AU DÉPART)
    for (const FNodeData& NodeData : LevelData->Nodes)
    {
        UHGONodeGraphComponent* NodeComp = NewObject<UHGONodeGraphComponent>(this, NodeGraphClass);

        if (!NodeComp)
            continue;

        NodeComp->SetupAttachment(GetRootComponent());
        NodeComp->RegisterComponent();
        
        NodeComp->SetWorldLocation(NodeData.Position);
        NodeComp->NodeData = NodeData;

        // CACHER TOUTES LES NODES AU DÉPART (scale 0)
        NodeComp->SetWorldScale3D(FVector::ZeroVector);
        NodeComp->SetVisibility(true, true);

        NodeGraphs.Add(NodeComp);
        SpawnedNodeMap.Add(NodeData.NodeID, NodeComp);
    }
    
    // GÉNÉRER LES EDGES (CACHÉES AU DÉPART)
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

        FVector EdgeScale(Distance / 100.f, 0.0f, 1.f); // Y scale à 0 au départ
        EdgeComp->SetWorldScale3D(EdgeScale);
        EdgeComp->SetVisibility(true, true);

        EdgeGraphs.Add(EdgeComp);
    }

    UE_LOG(LogTemp, Log, TEXT("[LevelGen] Graph generated with %d nodes and %d edges (all hidden)"), 
        NodeGraphs.Num(), EdgeGraphs.Num());
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

    AnimationLayers.Empty();
    CurrentAnimLayer = 0;
    bIsAnimating = false;
}

void AHGOTacticalLevelGenerator::BuildAnimationLayers()
{
    AnimationLayers.Empty();

    // Trouver la node Start
    UHGONodeGraphComponent* StartNode = nullptr;
    for (UHGONodeGraphComponent* Node : NodeGraphs)
    {
        if (Node && Node->NodeData.NodeType == ENodeType::Start)
        {
            StartNode = Node;
            break;
        }
    }

    if (!StartNode)
    {
        UE_LOG(LogTemp, Error, TEXT("[GraphAnim] No Start node found!"));
        return;
    }

    // BFS pour calculer la distance depuis Start
    TMap<UHGONodeGraphComponent*, int32> DistanceMap;
    TQueue<UHGONodeGraphComponent*> Queue;
    
    Queue.Enqueue(StartNode);
    DistanceMap.Add(StartNode, 0);

    int32 MaxDistance = 0;

    while (!Queue.IsEmpty())
    {
        UHGONodeGraphComponent* Current;
        Queue.Dequeue(Current);

        int32 CurrentDistance = DistanceMap[Current];
        MaxDistance = FMath::Max(MaxDistance, CurrentDistance);

        // Parcourir les voisins
        for (auto& Pair : Current->ConnectedNodes)
        {
            UHGONodeGraphComponent* Neighbor = Pair.Value;
            if (Neighbor && !DistanceMap.Contains(Neighbor))
            {
                DistanceMap.Add(Neighbor, CurrentDistance + 1);
                Queue.Enqueue(Neighbor);
            }
        }
    }

    // Créer les layers
    AnimationLayers.SetNum(MaxDistance + 1);

    for (auto& Pair : DistanceMap)
    {
        UHGONodeGraphComponent* Node = Pair.Key;
        int32 Distance = Pair.Value;

        FNodeAnimationData AnimData;
        AnimData.Node = Node;
        AnimData.DistanceFromStart = Distance;
        AnimData.TargetScale = FVector(.02f); // Scale finale des nodes
        AnimData.CurrentAnimTime = 0.0f;

        // Trouver les edges connectées à cette node
        for (UHGOEdgeGraphComponent* Edge : EdgeGraphs)
        {
            if (!Edge) continue;

            for (UHGONodeGraphComponent* OtherNode : NodeGraphs)
            {
                if (!OtherNode) continue;

                for (auto& Connection : OtherNode->ConnectedNodes)
                {
                    if (Connection.Value == Node && 
                        OtherNode->NodeData.NodeID == Edge->EdgeData.SourceNodeID &&
                        Node->NodeData.NodeID == Edge->EdgeData.TargetNodeID)
                    {
                        AnimData.ConnectedEdges.AddUnique(Edge);
                    }
                }
            }
        }

        AnimationLayers[Distance].Add(AnimData);
    }

    UE_LOG(LogTemp, Log, TEXT("[GraphAnim] Built %d animation layers"), AnimationLayers.Num());
}

void AHGOTacticalLevelGenerator::PlayGraphAnimation(bool bReversed)
{
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
        FString::Printf(TEXT("[GraphAnim] Starting animation (Reversed: %s)"), 
            bReversed ? TEXT("YES") : TEXT("NO")));

    bReverseAnimation = bReversed;
    BuildAnimationLayers();

    if (AnimationLayers.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[GraphAnim] No animation layers!"));
        return;
    }

    bIsAnimating = true;
    CurrentAnimLayer = bReversed ? AnimationLayers.Num() - 1 : 0;
    LayerTimer = 0.0f;
}

void AHGOTacticalLevelGenerator::UpdateGraphAnimation(float DeltaTime)
{
    if (!bIsAnimating || AnimationLayers.Num() == 0)
        return;

    // Animer le layer actuel
    if (CurrentAnimLayer >= 0 && CurrentAnimLayer < AnimationLayers.Num())
    {
        AnimateLayer(AnimationLayers[CurrentAnimLayer], DeltaTime);
    }

    // Attendre avant de passer au layer suivant
    LayerTimer += DeltaTime;

    if (LayerTimer >= NodeScaleDuration + DelayBetweenLayers)
    {
        // Passer au layer suivant
        if (bReverseAnimation)
        {
            CurrentAnimLayer--;
            if (CurrentAnimLayer < 0)
            {
                // Animation terminée
                bIsAnimating = false;
                OnGraphAnimationComplete();
                return;
            }
        }
        else
        {
            CurrentAnimLayer++;
            if (CurrentAnimLayer >= AnimationLayers.Num())
            {
                // Animation terminée
                bIsAnimating = false;
                OnGraphAnimationComplete();
                return;
            }
        }

        LayerTimer = 0.0f;
    }
}

void AHGOTacticalLevelGenerator::AnimateLayer(TArray<FNodeAnimationData>& Layer, float DeltaTime)
{
    for (FNodeAnimationData& AnimData : Layer)
    {
        if (!AnimData.Node)
            continue;

        AnimData.CurrentAnimTime += DeltaTime;
        float Alpha = FMath::Clamp(AnimData.CurrentAnimTime / NodeScaleDuration, 0.0f, 1.0f);

        // Courbe ease out
        Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 1.5f);

        // Animer la node
        FVector NodeScale = bReverseAnimation ? 
            FMath::Lerp(AnimData.TargetScale, FVector::ZeroVector, Alpha) :
            FMath::Lerp(FVector::ZeroVector, AnimData.TargetScale, Alpha);

        AnimData.Node->SetWorldScale3D(NodeScale);

        // Animer les edges
        for (UHGOEdgeGraphComponent* Edge : AnimData.ConnectedEdges)
        {
            if (!Edge)
                continue;

            FVector EdgeScale = Edge->GetComponentScale();
            float TargetYScale = 0.01f;

            float NewYScale = bReverseAnimation ?
                FMath::Lerp(TargetYScale, 0.0f, Alpha) :
                FMath::Lerp(0.0f, TargetYScale, Alpha);

            EdgeScale.Y = NewYScale;
            Edge->SetWorldScale3D(EdgeScale);
        }
    }
}

void AHGOTacticalLevelGenerator::OnGraphAnimationComplete()
{
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
        TEXT("[GraphAnim] ✓ Animation COMPLETE"));

    switch (CurrentAnimState)
    {
        case EAnimationState::HidingGraph:
            // Graph caché, lancer l'animation de board flip
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
                TEXT("[Sequence] Graph hidden → Triggering board flip animation"));
            
            CurrentAnimState = EAnimationState::WaitingForBoardFlip;
            break;

        case EAnimationState::ShowingGraph:
            // Graph affiché, séquence terminée
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
                TEXT("[Sequence] ✓ WORLD SWITCH COMPLETE - Player input enabled"));
            
            CurrentAnimState = EAnimationState::None;
            OnSwitchWorldAnimCompleted.Broadcast();
            break;

        case EAnimationState::None:
            // Animation initiale (BeginPlay)
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
                TEXT("[Sequence] ✓ Initial graph shown - Player input enabled"));
            
            OnGraphAnimationCompleted.Broadcast();
            break;

        default:
            break;
    }
}

void AHGOTacticalLevelGenerator::StartWorldSwitchSequence()
{
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Magenta,
        TEXT("[Sequence] === STARTING WORLD SWITCH SEQUENCE ==="));

    CurrentAnimState = EAnimationState::HidingGraph;
    
    // Cacher le graph actuel (reverse animation)
    PlayGraphAnimation(true);
}

void AHGOTacticalLevelGenerator::OnBoardFlipAnimationComplete()
{
    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
        TEXT("[Sequence] Board flip complete → Showing new graph"));

    CurrentAnimState = EAnimationState::ShowingGraph;
    
    // Afficher le nouveau graph (normal animation)
    PlayGraphAnimation(false);
}

void AHGOTacticalLevelGenerator::BeginPlay()
{
	Super::BeginPlay();

	GenerateVisualGraph();
    
    // Lancer l'animation initiale du graph
    PlayGraphAnimation(false);

    OnBoardFlipAnimCompleted.AddDynamic(this, &AHGOTacticalLevelGenerator::OnBoardFlipAnimationComplete);
}

void AHGOTacticalLevelGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    UpdateGraphAnimation(DeltaTime);
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
