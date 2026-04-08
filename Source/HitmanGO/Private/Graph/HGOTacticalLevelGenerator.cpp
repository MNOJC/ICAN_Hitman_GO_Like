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

	// Sécurise un root component si l'acteur n'en a pas
	USceneComponent* RootComp = GetRootComponent();
	if (!RootComp)
	{
		RootComp = NewObject<USceneComponent>(this, TEXT("GeneratedRoot"));
		if (!RootComp)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create GeneratedRoot"));
			return;
		}

		RootComp->RegisterComponent();
		SetRootComponent(RootComp);
	}

	TMap<int32, UHGONodeGraphComponent*> SpawnedNodeMap;

	// =========================
	// GENERATE NODES
	// =========================
	for (const FNodeData& NodeData : LevelData->Nodes)
	{
		UHGONodeGraphComponent* NodeComp = NewObject<UHGONodeGraphComponent>(this, NodeGraphClass);

		if (!NodeComp)
			continue;

		NodeComp->SetupAttachment(RootComp);
		NodeComp->RegisterComponent();

		FVector WorldNodePos = NodeData.Position;
		WorldNodePos = FVector(WorldNodePos.X, WorldNodePos.Y, ZOffset);

		NodeComp->SetWorldLocation(WorldNodePos);

		// Transférer les données depuis le Data Asset
		NodeComp->NodeData = NodeData;

		// Appliquer le bon matériau selon le monde
		if (NodeData.bIsUpsideDownNode)
		{
			if (UpsideDownNodeMaterial)
			{
				NodeComp->SetMaterial(0, UpsideDownNodeMaterial);
			}
		}
		else
		{
			if (NormalNodeMaterial)
			{
				NodeComp->SetMaterial(0, NormalNodeMaterial);
			}
		}

		// Visible + scale 0 pour garder ton anim
		NodeComp->SetHiddenInGame(false, true);
		NodeComp->SetVisibility(true, true);
		NodeComp->SetWorldScale3D(FVector::ZeroVector);

		NodeGraphs.Add(NodeComp);
		SpawnedNodeMap.Add(NodeData.NodeID, NodeComp);

		UE_LOG(LogTemp, Log, TEXT("[LevelGen] Node %d spawned | UpsideDown=%s"),
			NodeData.NodeID,
			NodeData.bIsUpsideDownNode ? TEXT("true") : TEXT("false"));
	}

	// =========================
	// GENERATE EDGES
	// =========================
	for (const FEdgeData& EdgeData : LevelData->Edges)
	{
		UHGONodeGraphComponent** SourceNodePtr = SpawnedNodeMap.Find(EdgeData.SourceNodeID);
		UHGONodeGraphComponent** TargetNodePtr = SpawnedNodeMap.Find(EdgeData.TargetNodeID);

		if (!SourceNodePtr || !TargetNodePtr)
			continue;

		UHGONodeGraphComponent* SourceNode = *SourceNodePtr;
		UHGONodeGraphComponent* TargetNode = *TargetNodePtr;

		if (!SourceNode || !TargetNode)
			continue;

		// Construire les connexions gameplay
		SourceNode->ConnectedNodes.Add(EdgeData.Direction, TargetNode);

		if (EdgeData.bIsBidirectional)
		{
			const ENodeDirection Opposite = GetOppositeDirection(EdgeData.Direction);
			TargetNode->ConnectedNodes.Add(Opposite, SourceNode);
		}

		const FVector SourcePos = SourceNode->GetComponentLocation();
		const FVector TargetPos = TargetNode->GetComponentLocation();

		const FVector MidPoint = (SourcePos + TargetPos) * 0.5f;
		const FVector DirectionVector = TargetPos - SourcePos;
		const FRotator EdgeRotation = DirectionVector.Rotation();

		float Distance = DirectionVector.Size();
		Distance -= 2.0f;
		Distance = FMath::Max(Distance, 1.0f);

		UHGOEdgeGraphComponent* EdgeComp = NewObject<UHGOEdgeGraphComponent>(this, EdgeGraphClass);

		if (!EdgeComp)
			continue;

		EdgeComp->SetupAttachment(RootComp);
		EdgeComp->RegisterComponent();

		EdgeComp->SetWorldLocation(MidPoint);
		EdgeComp->SetWorldRotation(EdgeRotation);

		// On part du Data Asset
		FEdgeData FinalEdgeData = EdgeData;

		// Un edge est upside down si au moins une des deux nodes reliées est upside down
		FinalEdgeData.bIsUpsideDownEdge =
			SourceNode->NodeData.bIsUpsideDownNode ||
			TargetNode->NodeData.bIsUpsideDownNode;

		EdgeComp->EdgeData = FinalEdgeData;

		// Appliquer le bon matériau selon le monde de l'edge
		if (FinalEdgeData.bIsUpsideDownEdge)
		{
			if (UpsideDownEdgeMaterial)
			{
				EdgeComp->SetMaterial(0, UpsideDownEdgeMaterial);
			}
		}
		else
		{
			if (NormalEdgeMaterial)
			{
				EdgeComp->SetMaterial(0, NormalEdgeMaterial);
			}
		}

		const FVector EdgeScale(Distance / 100.f, 0.0f, Distance / 100.f);
		EdgeComp->SetWorldScale3D(EdgeScale);

		// Visible
		EdgeComp->SetHiddenInGame(false, true);
		EdgeComp->SetVisibility(true, true);

		EdgeGraphs.Add(EdgeComp);

		UE_LOG(LogTemp, Log, TEXT("[LevelGen] Edge %d spawned | %d -> %d | UpsideDown=%s"),
			FinalEdgeData.EdgeID,
			FinalEdgeData.SourceNodeID,
			FinalEdgeData.TargetNodeID,
			FinalEdgeData.bIsUpsideDownEdge ? TEXT("true") : TEXT("false"));
	}

	UE_LOG(LogTemp, Log, TEXT("[LevelGen] Graph generated with %d nodes and %d edges"),
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

	// Déterminer le monde actif
	bool bTargetUpsideDownWorld = false;

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AHGOPlayerPawn> PlayerItr(World); PlayerItr; ++PlayerItr)
		{
			AHGOPlayerPawn* Player = *PlayerItr;
			if (Player && Player->GraphMovementComponent)
			{
				bTargetUpsideDownWorld = Player->GraphMovementComponent->bInUpsideDownWorld;
				break;
			}
		}
	}

	GEngine->AddOnScreenDebugMessage(
		-1,
		2.f,
		FColor::Cyan,
		FString::Printf(TEXT("[GraphAnim] Building layers for %s world"),
			bTargetUpsideDownWorld ? TEXT("UPSIDE-DOWN") : TEXT("NORMAL"))
	);

	TArray<UHGONodeGraphComponent*> WorldNodes;
	TMap<int32, UHGONodeGraphComponent*> WorldNodeMap;

	for (UHGONodeGraphComponent* Node : NodeGraphs)
	{
		if (Node && Node->NodeData.bIsUpsideDownNode == bTargetUpsideDownWorld)
		{
			WorldNodes.Add(Node);
			WorldNodeMap.Add(Node->NodeData.NodeID, Node);
		}
	}

	if (WorldNodes.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[GraphAnim] No nodes in target world!"));
		return;
	}

	UHGONodeGraphComponent* PreferredStartNode = nullptr;

	for (UHGONodeGraphComponent* Node : WorldNodes)
	{
		if (Node && Node->NodeData.NodeType == ENodeType::Start)
		{
			PreferredStartNode = Node;
			break;
		}
	}

	if (!PreferredStartNode)
	{
		for (UHGONodeGraphComponent* Node : WorldNodes)
		{
			if (Node && Node->NodeData.NodeType == ENodeType::PlayerPortal)
			{
				PreferredStartNode = Node;
				break;
			}
		}
	}

	if (!PreferredStartNode)
	{
		PreferredStartNode = WorldNodes[0];
	}

	TSet<UHGONodeGraphComponent*> GlobalVisited;
	int32 LayerOffset = 0;

	auto AddEdgesForNode = [&](FNodeAnimationData& AnimData)
	{
		for (UHGOEdgeGraphComponent* Edge : EdgeGraphs)
		{
			if (!Edge)
				continue;

			// On utilise maintenant directement le bool calculé au spawn
			if (Edge->EdgeData.bIsUpsideDownEdge != bTargetUpsideDownWorld)
				continue;

			if (Edge->EdgeData.SourceNodeID == AnimData.Node->NodeData.NodeID ||
				Edge->EdgeData.TargetNodeID == AnimData.Node->NodeData.NodeID)
			{
				AnimData.ConnectedEdges.AddUnique(Edge);
			}
		}
	};

	auto BuildComponentFromStart = [&](UHGONodeGraphComponent* ComponentStart)
	{
		if (!ComponentStart || GlobalVisited.Contains(ComponentStart))
			return;

		TMap<UHGONodeGraphComponent*, int32> DistanceMap;
		TQueue<UHGONodeGraphComponent*> Queue;

		Queue.Enqueue(ComponentStart);
		DistanceMap.Add(ComponentStart, 0);
		GlobalVisited.Add(ComponentStart);

		int32 MaxDistanceInComponent = 0;

		while (!Queue.IsEmpty())
		{
			UHGONodeGraphComponent* Current = nullptr;
			Queue.Dequeue(Current);

			const int32 CurrentDistance = DistanceMap[Current];
			MaxDistanceInComponent = FMath::Max(MaxDistanceInComponent, CurrentDistance);

			for (const auto& Pair : Current->ConnectedNodes)
			{
				UHGONodeGraphComponent* Neighbor = Pair.Value;

				if (Neighbor &&
					Neighbor->NodeData.bIsUpsideDownNode == bTargetUpsideDownWorld &&
					!DistanceMap.Contains(Neighbor))
				{
					DistanceMap.Add(Neighbor, CurrentDistance + 1);
					GlobalVisited.Add(Neighbor);
					Queue.Enqueue(Neighbor);
				}
			}
		}

		const int32 NeededLayerCount = LayerOffset + MaxDistanceInComponent + 1;
		if (AnimationLayers.Num() < NeededLayerCount)
		{
			AnimationLayers.SetNum(NeededLayerCount);
		}

		for (const auto& Pair : DistanceMap)
		{
			UHGONodeGraphComponent* Node = Pair.Key;
			const int32 LocalDistance = Pair.Value;
			const int32 FinalLayerIndex = LayerOffset + LocalDistance;

			FNodeAnimationData AnimData;
			AnimData.Node = Node;
			AnimData.DistanceFromStart = FinalLayerIndex;
			AnimData.TargetScale = FVector(0.06f);
			AnimData.CurrentAnimTime = 0.0f;

			AddEdgesForNode(AnimData);

			AnimationLayers[FinalLayerIndex].Add(AnimData);
		}

		UE_LOG(LogTemp, Log, TEXT("[GraphAnim] Component built from node %d with %d layers"),
			ComponentStart->NodeData.NodeID,
			MaxDistanceInComponent + 1);

		LayerOffset += MaxDistanceInComponent + 1;
	};

	BuildComponentFromStart(PreferredStartNode);

	for (UHGONodeGraphComponent* Node : WorldNodes)
	{
		if (Node && !GlobalVisited.Contains(Node))
		{
			BuildComponentFromStart(Node);
		}
	}

	GEngine->AddOnScreenDebugMessage(
		-1,
		2.f,
		FColor::Green,
		FString::Printf(TEXT("[GraphAnim] Built %d layers for %s world (%d nodes total)"),
			AnimationLayers.Num(),
			bTargetUpsideDownWorld ? TEXT("UPSIDE-DOWN") : TEXT("NORMAL"),
			WorldNodes.Num())
	);

	for (int32 i = 0; i < AnimationLayers.Num(); ++i)
	{
		UE_LOG(LogTemp, Log, TEXT("[GraphAnim] Layer %d: %d nodes"), i, AnimationLayers[i].Num());
	}
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

	if (CurrentAnimLayer >= 0 && CurrentAnimLayer < AnimationLayers.Num())
	{
		AnimateLayer(AnimationLayers[CurrentAnimLayer], DeltaTime);
	}

	LayerTimer += DeltaTime;

	if (LayerTimer >= NodeScaleDuration + DelayBetweenLayers)
	{
		if (bReverseAnimation)
		{
			CurrentAnimLayer--;
			if (CurrentAnimLayer < 0)
			{
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

		Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 1.5f);

		FVector NodeScale = bReverseAnimation
			? FMath::Lerp(AnimData.TargetScale, FVector::ZeroVector, Alpha)
			: FMath::Lerp(FVector::ZeroVector, AnimData.TargetScale, Alpha);

		AnimData.Node->SetWorldScale3D(NodeScale);

		for (UHGOEdgeGraphComponent* Edge : AnimData.ConnectedEdges)
		{
			if (!Edge)
				continue;

			FVector EdgeScale = Edge->GetComponentScale();
			const float TargetYScale = 0.01f;

			const float NewYScale = bReverseAnimation
				? FMath::Lerp(TargetYScale, 0.0f, Alpha)
				: FMath::Lerp(0.0f, TargetYScale, Alpha);

			EdgeScale.Y = NewYScale;
			Edge->SetWorldScale3D(EdgeScale);
		}
	}
}

void AHGOTacticalLevelGenerator::OnGraphAnimationComplete()
{
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
		TEXT("[GraphAnim] Animation COMPLETE"));

	switch (CurrentAnimState)
	{
	case EAnimationState::HidingGraph:
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
			TEXT("[Sequence] Graph hidden -> Triggering board flip animation"));

		CurrentAnimState = EAnimationState::WaitingForBoardFlip;
		break;

	case EAnimationState::ShowingGraph:
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
			TEXT("[Sequence] WORLD SWITCH COMPLETE - Player input enabled"));

		CurrentAnimState = EAnimationState::None;
		OnSwitchWorldAnimCompleted.Broadcast();
		break;

	case EAnimationState::None:
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
			TEXT("[Sequence] Initial graph shown - Player input enabled"));

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
	PlayGraphAnimation(true);
}

void AHGOTacticalLevelGenerator::OnBoardFlipAnimationComplete()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
		TEXT("[Sequence] Board flip complete -> Showing new graph"));

	CurrentAnimState = EAnimationState::ShowingGraph;
	PlayGraphAnimation(false);
}

void AHGOTacticalLevelGenerator::BeginPlay()
{
	Super::BeginPlay();

	GenerateVisualGraph();
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