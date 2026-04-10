// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/HGOEnemyPawn.h"
#include "Graph/HGOTacticalLevelGenerator.h"
#include "EngineUtils.h"
#include "Core/HGOTacticalTurnManager.h"

namespace
{
	bool IsPlayerInWorld(UWorld* World, bool bUpsideDownWorld)
	{
		if (!World)
		{
			return false;
		}

		for (TActorIterator<AHGOPlayerPawn> PlayerItr(World); PlayerItr; ++PlayerItr)
		{
			const AHGOPlayerPawn* Player = *PlayerItr;
			if (Player && Player->GraphMovementComponent)
			{
				return Player->GraphMovementComponent->bInUpsideDownWorld == bUpsideDownWorld;
			}
		}

		return false;
	}

	UHGONodeGraphComponent* FindLinkedPortalNode(UWorld* World, const AHGOEnemyPawn* EnemyPawn, bool bTargetWorld)
	{
		if (!World || !EnemyPawn || !EnemyPawn->GraphMovementComponent || !EnemyPawn->GraphMovementComponent->GetCurrentNode())
		{
			return nullptr;
		}

		UHGONodeGraphComponent* CurrentPortalNode = EnemyPawn->GraphMovementComponent->GetCurrentNode();
		const int32 LinkedID = CurrentPortalNode->NodeData.LinkedUpsideDownNodeID;

		if (LinkedID < 0)
		{
			return nullptr;
		}

		AHGOTacticalLevelGenerator* Generator = nullptr;
		for (TActorIterator<AHGOTacticalLevelGenerator> It(World); It; ++It)
		{
			Generator = *It;
			break;
		}

		if (!Generator)
		{
			return nullptr;
		}

		for (UHGONodeGraphComponent* NodeComp : Generator->NodeGraphs)
		{
			if (!NodeComp)
			{
				continue;
			}

			if (NodeComp->NodeData.LinkedUpsideDownNodeID == LinkedID
				&& NodeComp->NodeData.bIsUpsideDownNode == bTargetWorld
				&& NodeComp->NodeData.NodeType == ENodeType::EnemyPortal)
			{
				return NodeComp;
			}
		}

		return nullptr;
	}
}

AHGOEnemyPawn::AHGOEnemyPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	EnemyMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyMeshComponent"));
	EnemyMeshComponent->SetupAttachment(SceneRoot);

	DetectionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("DetectionCollision"));
	DetectionCollision->SetupAttachment(SceneRoot);
	DetectionCollision->SetBoxExtent(FVector(40.f, 40.f, 40.f));
	DetectionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	GraphMovementComponent = CreateDefaultSubobject<UHGOGraphMovementComponent>(TEXT("GraphMovementComponent"));
}

void AHGOEnemyPawn::BeginPlay()
{
	Super::BeginPlay();

	InitEnemyPosition();

	DetectionCollision->OnComponentBeginOverlap.AddDynamic(this, &AHGOEnemyPawn::OnDetectionOverlapBegin);
	
}

void AHGOEnemyPawn::InitEnemyPosition()
{
	if (MovementPathNodeIDs.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No movement path defined!"));
		return;
	}
	
	AHGOTacticalLevelGenerator* Generator = nullptr;
	for (TActorIterator<AHGOTacticalLevelGenerator> GeneratorItr(GetWorld()); GeneratorItr; ++GeneratorItr)
	{
		Generator = *GeneratorItr;
		break;
	}

	if (!Generator)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No level generator found!"));
		return;
	}
	
	int32 StartNodeID = MovementPathNodeIDs[0];
	UHGONodeGraphComponent* StartNode = nullptr;

	for (UHGONodeGraphComponent* NodeComp : Generator->NodeGraphs)
	{
		if (NodeComp && NodeComp->NodeData.NodeID == StartNodeID)
		{
			StartNode = NodeComp;
			break;
		}
	}

	if (StartNode)
	{
		GraphMovementComponent->SetCurrentNode(StartNode);
		this->SetActorRotation(DefaultRotation);
		UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Initialized at node %d"), StartNodeID);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] Could not find start node %d"), StartNodeID);
	}
}

void AHGOEnemyPawn::ExecuteEnemyMove()
{
	UFMODBlueprintStatics::PlayEvent2D(GetWorld(), EnemyUpSound, true);
	if (MovementPathNodeIDs.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No movement path defined!"));
		return;
	}

	if (!GraphMovementComponent || !GraphMovementComponent->GetCurrentNode())
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No current node!"));
		return;
	}
	
	if (bBeingPushed)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
			TEXT("[Enemy] Being pushed - skipping normal move"));
		
		if (UWorld* World = GetWorld())
		{
			if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
			{
				TurnManager->RegisterActionStarted();
				TurnManager->RegisterActionCompleted();
			}
		}
		return;
	}
	
	if (bReturningToPatrol)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
			TEXT("[Enemy] Returning to patrol"));
		
		if (PushPathNodeIDs.Num() > 1)
		{
			PushPathNodeIDs.RemoveAt(PushPathNodeIDs.Num() - 1);
			
			int32 NextNodeID = PushPathNodeIDs.Last();

			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
				FString::Printf(TEXT("[Enemy] Moving back to node %d (%d nodes remaining)"),
					NextNodeID, PushPathNodeIDs.Num() - 1));

			if (GraphMovementComponent->TryMoveToNodeID(NextNodeID))
			{
				if (IsNodeInPatrol(NextNodeID))
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
						TEXT("[Enemy] ★ BACK ON PATROL ★"));

					bReturningToPatrol = false;
					PushPathNodeIDs.Empty();
					
					for (int32 i = 0; i < MovementPathNodeIDs.Num(); ++i)
					{
						if (MovementPathNodeIDs[i] == NextNodeID)
						{
							CurrentPathIndex = i;
							GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
								FString::Printf(TEXT("[Enemy] Resuming patrol at index %d"), i));
							break;
						}
					}
				}
			}
			return;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Current node ID: %d | Type: %s"),
		GraphMovementComponent->GetCurrentNode()->NodeData.NodeID,
		*UEnum::GetValueAsString(GraphMovementComponent->GetCurrentNode()->NodeData.NodeType));
	
	if (GraphMovementComponent->GetCurrentNode()->NodeData.NodeType == ENodeType::EnemyPortal)
	{
		if (bJustCrossedPortal)
		{
			bJustCrossedPortal = false;
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Purple,
				TEXT("[Enemy] On portal node but just crossed - moving normally"));
		}
		else
		{
			HandleEnemyPortal();
			return;
		}
	}
	
	int32 TargetNodeID = GetNextNodeID();

	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Moving to next node in path: %d (index %d/%d)"),
		TargetNodeID, CurrentPathIndex, MovementPathNodeIDs.Num() - 1);

	if (GraphMovementComponent->TryMoveToNodeID(TargetNodeID))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Successfully started move to node %d"), TargetNodeID);
		AdvancePathIndex();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Failed to move to node %d"), TargetNodeID);
	}
}

void AHGOEnemyPawn::ExecuteEnemyRotation()
{
	if (!GraphMovementComponent || !GraphMovementComponent->GetCurrentNode())
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] Cannot calculate rotation - no current node"));
		return;
	}

	int32 NextNodeID = GetNextNodeID();
	
	if (GraphMovementComponent->GetCurrentNode()->NodeData.NodeType == ENodeType::EnemyPortal
		&& PortalState == EEnemyPortalState::Building)
	{
		int32 SimulatedIndex = CurrentPathIndex;
		
		if (PathFollowType == EPathFollowType::Loop)
		{
			SimulatedIndex = (SimulatedIndex + 1) % MovementPathNodeIDs.Num();
		}
		else
		{
			if (bReverseDirection)
			{
				SimulatedIndex = FMath::Max(0, SimulatedIndex - 1);
			}
			else
			{
				SimulatedIndex = FMath::Min(MovementPathNodeIDs.Num() - 1, SimulatedIndex + 1);
			}
		}
		
		if (PathFollowType == EPathFollowType::Loop)
		{
			SimulatedIndex = (SimulatedIndex + 1) % MovementPathNodeIDs.Num();
		}
		else 
		{
			if (bReverseDirection)
			{
				SimulatedIndex = FMath::Max(0, SimulatedIndex - 1);
			}
			else
			{
				SimulatedIndex = FMath::Min(MovementPathNodeIDs.Num() - 1, SimulatedIndex + 1);
			}
		}

		NextNodeID = MovementPathNodeIDs[SimulatedIndex];
		UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] On portal - rotating towards node AFTER crossing: %d"), NextNodeID);
	}
	
	AHGOTacticalLevelGenerator* Generator = nullptr;
	for (TActorIterator<AHGOTacticalLevelGenerator> GeneratorItr(GetWorld()); GeneratorItr; ++GeneratorItr)
	{
		Generator = *GeneratorItr;
		break;
	}

	if (!Generator)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No level generator found for rotation"));
		return;
	}

	UHGONodeGraphComponent* NextNode = nullptr;
	for (UHGONodeGraphComponent* NodeComp : Generator->NodeGraphs)
	{
		if (NodeComp && NodeComp->NodeData.NodeID == NextNodeID)
		{
			NextNode = NodeComp;
			break;
		}
	}

	if (!NextNode)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] Next node %d not found"), NextNodeID);
		return;
	}
	
	FVector CurrentPos = GraphMovementComponent->GetCurrentNode()->GetComponentLocation();
	FVector NextPos = NextNode->GetComponentLocation();
	FVector Direction = (NextPos - CurrentPos).GetSafeNormal();
	
	FRotator TargetRotation = Direction.Rotation();
	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;

	NextRotation = TargetRotation;
	bIsRotating = true;

	UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Rotating to face node %d (Yaw: %.1f)"),
		NextNodeID, TargetRotation.Yaw);
}

void AHGOEnemyPawn::UpdateEnemyRotation(float DeltaTime)
{
	if (bIsRotating)
	{
		FRotator TargetRotation = FMath::RInterpConstantTo(GetActorRotation(), NextRotation, DeltaTime, 180.f);
		this->SetActorRotation(TargetRotation);

		if (TargetRotation.Equals(NextRotation, 1.0f))
		{
			bIsRotating = false;
			UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Rotation complete"));
			if (UWorld* World = GetWorld())
			{
				if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
				{
					TurnManager->RegisterActionCompleted();
				}
			}
		}
	}
}

void AHGOEnemyPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateEnemyRotation(DeltaTime);
	UpdatePortalDive(DeltaTime);
}

void AHGOEnemyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AHGOEnemyPawn::AdvancePathIndex()
{
	if (PathFollowType == EPathFollowType::Loop)
	{
		CurrentPathIndex = (CurrentPathIndex + 1) % MovementPathNodeIDs.Num();
	}
	else 
	{
		if (bReverseDirection)
		{
			CurrentPathIndex--;
			if (CurrentPathIndex <= 0)
			{
				CurrentPathIndex = 0;
				bReverseDirection = false;
			}
		}
		else
		{
			CurrentPathIndex++;
			if (CurrentPathIndex >= MovementPathNodeIDs.Num() - 1)
			{
				CurrentPathIndex = MovementPathNodeIDs.Num() - 1;
				bReverseDirection = true;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Advanced to path index %d (Reverse: %s)"),
		CurrentPathIndex, bReverseDirection ? TEXT("Yes") : TEXT("No"));
}

int32 AHGOEnemyPawn::GetNextNodeID()
{
	int32 NextIndex = CurrentPathIndex;

	if (PathFollowType == EPathFollowType::Loop)
	{
		NextIndex = (CurrentPathIndex + 1) % MovementPathNodeIDs.Num();
	}
	else 
	{
		if (bReverseDirection)
		{
			NextIndex = FMath::Max(0, CurrentPathIndex - 1);
		}
		else
		{
			NextIndex = FMath::Min(MovementPathNodeIDs.Num() - 1, CurrentPathIndex + 1);
		}
	}

	return MovementPathNodeIDs[NextIndex];
}

void AHGOEnemyPawn::HandlePortalOnArrival()
{
	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Arrived at portal - building portal this turn"));
	BuildPortal();
}

void AHGOEnemyPawn::HandleEnemyPortal()
{
	switch (PortalState)
	{
	case EEnemyPortalState::None:
		BuildPortal();
		break;

	case EEnemyPortalState::Building:
		StartPortalDive();
		break;

	default:
		break;
	}
}

void AHGOEnemyPawn::BuildPortal()
{
	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Building portal - 1 turn used"));

	PortalState = EEnemyPortalState::Building;

	if (GraphMovementComponent && GraphMovementComponent->GetCurrentNode())
	{
		const int32 CreatedPortalNodeID = GraphMovementComponent->GetCurrentNode()->NodeData.NodeID;
		OnPortalCreated.Broadcast(CreatedPortalNodeID);

		UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] OnPortalCreated broadcast with node ID %d"), CreatedPortalNodeID);
	}
	
	if (UWorld* World = GetWorld())
	{
		if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
		{
			TurnManager->RegisterActionStarted();
			UFMODBlueprintStatics::PlayEvent2D(GetWorld(), PortalCreateSound, true);
			ExecuteEnemyRotation();
		}
	}
	
}

void AHGOEnemyPawn::CrossPortal()
{
	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Crossing portal to other world"));

	UHGONodeGraphComponent* CurrentPortalNode = GraphMovementComponent->GetCurrentNode();
	if (!CurrentPortalNode)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] CrossPortal: no current node!"));
		return;
	}

	int32 LinkedID = CurrentPortalNode->NodeData.LinkedUpsideDownNodeID;
	if (LinkedID < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] CrossPortal: no LinkedUpsideDownNodeID on current portal node!"));
		return;
	}
	
	bool bTargetWorld = !bInUpsideDownWorld;

	UHGONodeGraphComponent* LinkedPortalNode = FindLinkedPortalNode(GetWorld(), this, bTargetWorld);

	if (LinkedPortalNode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Found linked portal node %d in %s world"),
			LinkedPortalNode->NodeData.NodeID,
			bTargetWorld ? TEXT("UPSIDE-DOWN") : TEXT("NORMAL"));
	}

	if (!LinkedPortalNode)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] CrossPortal: could not find linked portal node with LinkedID=%d in world %s"),
			LinkedID, bTargetWorld ? TEXT("UPSIDE-DOWN") : TEXT("NORMAL"));
		if (UWorld* World = GetWorld())
		{
			if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
			{
				TurnManager->RegisterActionStarted();
				TurnManager->RegisterActionCompleted();
			}
		}
		return;
	}

	OnPortalCrossed.Broadcast(GraphMovementComponent->GetCurrentNode()->NodeData.NodeID);
	
	bInUpsideDownWorld = bTargetWorld;
	GraphMovementComponent->bInUpsideDownWorld = bTargetWorld;
	GraphMovementComponent->SetCurrentNode(LinkedPortalNode);

	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Teleported to portal node %d in %s world — will move normally next turn"),
		LinkedPortalNode->NodeData.NodeID, bInUpsideDownWorld ? TEXT("UPSIDE-DOWN") : TEXT("NORMAL"));

	if (!bPortalVisualStateAppliedBeforeCross)
	{
		OnEnemyPassThroughPortal();
	}

	bPortalVisualStateAppliedBeforeCross = false;
	
	PortalState = EEnemyPortalState::None;
	
	bJustCrossedPortal = true;
	
	AdvancePathIndex();
	CheckAndKillPlayer();
	
	if (UWorld* World = GetWorld())
	{
		if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
		{
			TurnManager->RegisterActionStarted();
			TurnManager->RegisterActionCompleted();
		}
	}
}

void AHGOEnemyPawn::UpdateVisibilityForWorld(bool bPlayerInUpsideDownWorld)
{
	OnEnemyPassThroughPortal();
	bool bShouldBeVisible = (bInUpsideDownWorld == bPlayerInUpsideDownWorld);
	
	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Visibility updated: %s (Player in %s, Enemy in %s)"),
		bShouldBeVisible ? TEXT("VISIBLE") : TEXT("HIDDEN"),
		bPlayerInUpsideDownWorld ? TEXT("UPSIDE-DOWN") : TEXT("NORMAL"),
		bInUpsideDownWorld ? TEXT("UPSIDE-DOWN") : TEXT("NORMAL"));
}

bool AHGOEnemyPawn::OnEnemyPassThroughPortal_Implementation()
{
	return bInUpsideDownWorld;
}

void AHGOEnemyPawn::OnDetectionOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AHGOPlayerPawn* Player = Cast<AHGOPlayerPawn>(OtherActor);
	if (!Player)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Player walked onto the same tile! Killing player (world-agnostic overlap)."));
	Player->KillPlayer(true);
}

void AHGOEnemyPawn::StartPortalDive()
{
	if (bIsPortalDiving)
	{
		return;
	}
	
	bPortalVisualStateAppliedBeforeCross = false;
	
	if (PortalDiveDuration <= KINDA_SMALL_NUMBER)
	{
		CrossPortal();
		return;
	}

	UFMODBlueprintStatics::PlayEvent2D(GetWorld(), EnemyCrossPortalSound, true);

	bIsPortalDiving = true;
	PortalDiveElapsed = 0.0f;

	const bool bTargetWorld = !bInUpsideDownWorld;
	const bool bPlayerInTargetWorld = IsPlayerInWorld(GetWorld(), bTargetWorld);
	
	if (bPlayerInTargetWorld)
	{
		if (UHGONodeGraphComponent* LinkedPortalNode = FindLinkedPortalNode(GetWorld(), this, bTargetWorld))
		{
			const FVector FinalLocation = LinkedPortalNode->GetComponentLocation();

			PortalDiveStartLocation = FinalLocation + FVector(0.0f, 0.0f, PortalDiveOffsetZ);
			PortalDiveTargetLocation = FinalLocation;
			
			SetActorLocation(PortalDiveStartLocation);
			
			const bool bPreviousEnemyWorld = bInUpsideDownWorld;
			const bool bPreviousMovementWorld = GraphMovementComponent
				? GraphMovementComponent->bInUpsideDownWorld
				: bInUpsideDownWorld;

			bInUpsideDownWorld = bTargetWorld;
			if (GraphMovementComponent)
			{
				GraphMovementComponent->bInUpsideDownWorld = bTargetWorld;
			}

			OnEnemyPassThroughPortal();
			
			bInUpsideDownWorld = bPreviousEnemyWorld;
			if (GraphMovementComponent)
			{
				GraphMovementComponent->bInUpsideDownWorld = bPreviousMovementWorld;
			}

			bPortalVisualStateAppliedBeforeCross = true;

			UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Starting portal rise transition into player's world"));
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Rise transition fallback: linked portal node not found"));
	}
	
	PortalDiveStartLocation = GetActorLocation();
	PortalDiveTargetLocation = PortalDiveStartLocation + FVector(0.0f, 0.0f, PortalDiveOffsetZ);

	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Starting portal dive transition"));
}

void AHGOEnemyPawn::UpdatePortalDive(float DeltaTime)
{
	if (!bIsPortalDiving)
	{
		return;
	}

	PortalDiveElapsed += DeltaTime;

	const float Alpha = FMath::Clamp(PortalDiveElapsed / PortalDiveDuration, 0.0f, 1.0f);
	const float SmoothedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

	const FVector NewLocation = FMath::Lerp(PortalDiveStartLocation, PortalDiveTargetLocation, SmoothedAlpha);
	SetActorLocation(NewLocation);

	if (Alpha >= 1.0f)
	{
		bIsPortalDiving = false;

		CrossPortal();
	}
}

bool AHGOEnemyPawn::CheckAndKillPlayer()
{
	if (!GraphMovementComponent || !GraphMovementComponent->GetCurrentNode())
	{
		return false;
	}
	
	AHGOPlayerPawn* Player = nullptr;
	for (TActorIterator<AHGOPlayerPawn> PlayerItr(GetWorld()); PlayerItr; ++PlayerItr)
	{
		Player = *PlayerItr;
		break;
	}

	if (!Player || !Player->GraphMovementComponent)
	{
		return false;
	}

	UHGONodeGraphComponent* PlayerNode = Player->GraphMovementComponent->GetCurrentNode();
	if (!PlayerNode)
	{
		return false;
	}
	
	if (bInUpsideDownWorld != Player->GraphMovementComponent->bInUpsideDownWorld)
	{
		return false;
	}
	
	if (GraphMovementComponent->GetCurrentNode() == PlayerNode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Reached player tile. Killing player."));
		bKillMoveInProgress = false;
		Player->KillPlayer(false);
		return true;
	}
	
	if (GraphMovementComponent->IsNodeAdjacent(PlayerNode))
	{
		if (GraphMovementComponent->TryMoveToNodeID(PlayerNode->NodeData.NodeID))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Player detected. Starting kill move."));
			bKillMoveInProgress = true;
			return true;
		}
	}

	return false;
}

void AHGOEnemyPawn::PushEnemy(ENodeDirection Direction)
{
	GEngine->AddOnScreenDebugMessage(
		-1, 3.f, FColor::Magenta,
		FString::Printf(TEXT("[Push] PushEnemy called with direction: %s"),
			*UEnum::GetValueAsString(Direction))
	);

	if (!GraphMovementComponent || !GraphMovementComponent->GetCurrentNode())
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
			TEXT("[Push] FAILED - No current node"));
		return;
	}
	
	AHGOTacticalLevelGenerator* Generator = nullptr;
	for (TActorIterator<AHGOTacticalLevelGenerator> GeneratorItr(GetWorld()); GeneratorItr; ++GeneratorItr)
	{
		Generator = *GeneratorItr;
		break;
	}

	if (!Generator)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
			TEXT("[Push] FAILED - No level generator"));
		return;
	}
	
	UHGONodeGraphComponent* StartNode = GraphMovementComponent->GetCurrentNode();
	LastPatrolNodeID = StartNode->NodeData.NodeID;
	
	UHGONodeGraphComponent* CurrentCheck = StartNode;
	UHGONodeGraphComponent* LastValidNode = StartNode;
	PushPathNodeIDs.Empty();
	PushPathNodeIDs.Add(StartNode->NodeData.NodeID);
	
	for (int32 i = 0; i < 20; ++i)
	{
		UHGONodeGraphComponent* NextNode = CurrentCheck->GetNodeInDirection(Direction);

		if (!NextNode)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
				FString::Printf(TEXT("[Push] Dead-end after %d nodes"), PushPathNodeIDs.Num() - 1));
			break;
		}
		
		LastValidNode = NextNode;
		PushPathNodeIDs.Add(NextNode->NodeData.NodeID);
		CurrentCheck = NextNode;

		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
			FString::Printf(TEXT("[Push] Found connected node: %d"), NextNode->NodeData.NodeID));
	}
	
	if (PushPathNodeIDs.Num() <= 1)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
			TEXT("[Push] No node to push to - ability consumed but enemy doesn't move"));
		return;
	}


	bBeingPushed = true;
	bJustCrossedPortal = false; 

	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
		FString::Printf(TEXT("[Push] Starting push - will move %d nodes"), PushPathNodeIDs.Num() - 1));
	
	int32 NextNodeID = PushPathNodeIDs[1]; 

	if (GraphMovementComponent->TryMoveToNodeID(NextNodeID))
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
			FString::Printf(TEXT("[Push] Started moving to node %d"), NextNodeID));
	}
}

bool AHGOEnemyPawn::IsNodeInPatrol(int32 NodeID) const
{
	return MovementPathNodeIDs.Contains(NodeID);
}

void AHGOEnemyPawn::StartReturnToPatrol()
{
	bReturningToPatrol = true;

	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
		FString::Printf(TEXT("[Return] Starting return to patrol (%d nodes to backtrack)"),
			PushPathNodeIDs.Num() - 1));
}