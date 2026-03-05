// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/HGOEnemyPawn.h"
#include "Graph/HGOTacticalLevelGenerator.h"
#include "EngineUtils.h"
#include "Core/HGOTacticalTurnManager.h"

// Sets default values
AHGOEnemyPawn::AHGOEnemyPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	EnemyMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyMeshComponent"));
	EnemyMeshComponent->SetupAttachment(SceneRoot);

	// Detection collision : tue le joueur s'il marche sur la même case, même si l'ennemi est invisible
	DetectionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("DetectionCollision"));
	DetectionCollision->SetupAttachment(SceneRoot);
	DetectionCollision->SetBoxExtent(FVector(40.f, 40.f, 40.f));
	DetectionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	GraphMovementComponent = CreateDefaultSubobject<UHGOGraphMovementComponent>(TEXT("GraphMovementComponent"));
}

// Called when the game starts or when spawned
void AHGOEnemyPawn::BeginPlay()
{
	Super::BeginPlay();
	
	InitEnemyPosition();

	// Binder l'overlap pour tuer le joueur quand il entre sur la même case (peu importe le monde)
	DetectionCollision->OnComponentBeginOverlap.AddDynamic(this, &AHGOEnemyPawn::OnDetectionOverlapBegin);
	
	// S'abonner au changement de monde du joueur
	if (UWorld* World = GetWorld())
	{
		if (AHGOGameMode* GameMode = World->GetAuthGameMode<AHGOGameMode>())
		{
			GameMode->OnSwitchWorldGraph.AddDynamic(this, &AHGOEnemyPawn::UpdateVisibilityForWorld);
		}
	}
}

void AHGOEnemyPawn::InitEnemyPosition()
{
	if (MovementPathNodeIDs.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No movement path defined!"));
		return;
	}

	// Find the level generator
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

	// Find the start node
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
		UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Initialized at node %d"), StartNodeID);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] Could not find start node %d"), StartNodeID);
	}
}

void AHGOEnemyPawn::ExecuteEnemyMove()
{
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

	// CAS 1: L'ennemi est en train d'être poussé - NE PAS BOUGER
	if (bBeingPushed)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
			TEXT("[Enemy] Being pushed - skipping normal move"));
		
		// Terminer le tour sans bouger
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

	// CAS 2: L'ennemi retourne à sa patrol
	if (bReturningToPatrol)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
			TEXT("[Enemy] Returning to patrol"));

		// Trouver la node suivante dans le chemin de retour
		if (PushPathNodeIDs.Num() > 1)
		{
			// Retirer la node actuelle
			PushPathNodeIDs.RemoveAt(PushPathNodeIDs.Num() - 1);
			
			// Obtenir la prochaine node dans le chemin inversé
			int32 NextNodeID = PushPathNodeIDs.Last();
			
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
				FString::Printf(TEXT("[Enemy] Moving back to node %d (%d nodes remaining)"), 
					NextNodeID, PushPathNodeIDs.Num() - 1));

			if (GraphMovementComponent->TryMoveToNodeID(NextNodeID))
			{
				// Vérifier si on est revenu sur la patrol
				if (IsNodeInPatrol(NextNodeID))
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
						TEXT("[Enemy] ★ BACK ON PATROL ★"));
					
					bReturningToPatrol = false;
					PushPathNodeIDs.Empty();
					
					// Trouver l'index dans la patrol
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

	// CAS 3: Mouvement normal de patrol
	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Current node ID: %d | Type: %s"), 
		GraphMovementComponent->GetCurrentNode()->NodeData.NodeID, 
		*UEnum::GetValueAsString(GraphMovementComponent->GetCurrentNode()->NodeData.NodeType));
	
	// Vérifier si on est sur un portail ennemi
	if (GraphMovementComponent->GetCurrentNode()->NodeData.NodeType == ENodeType::EnemyPortal)
	{
		HandleEnemyPortal();
		return;
	}

	// Mouvement normal
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
	
	// Si on est sur un portail et qu'on construit, on veut se tourner vers la node APRÈS le portail
	// car la prochaine node (dans l'autre monde) est au même endroit
	if (GraphMovementComponent->GetCurrentNode()->NodeData.NodeType == ENodeType::EnemyPortal 
		&& PortalState == EEnemyPortalState::Building)
	{
		// Calculer la node après le portail
		// GetNextNodeID() retourne la node dans l'autre monde (même position)
		// On veut la node d'APRÈS
		
		// Simuler l'avancement comme dans CrossPortal
		int32 SimulatedIndex = CurrentPathIndex;
		
		// Avancer une fois (vers la node du portail dans l'autre monde)
		if (PathFollowType == EPathFollowType::Loop)
		{
			SimulatedIndex = (SimulatedIndex + 1) % MovementPathNodeIDs.Num();
		}
		else // PingPong
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
		
		// Avancer encore une fois (vers la node réelle après le portail)
		if (PathFollowType == EPathFollowType::Loop)
		{
			SimulatedIndex = (SimulatedIndex + 1) % MovementPathNodeIDs.Num();
		}
		else // PingPong
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

	// Trouver la node correspondante
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

	// Calculer la direction vers la prochaine node
	FVector CurrentPos = GraphMovementComponent->GetCurrentNode()->GetComponentLocation();
	FVector NextPos = NextNode->GetComponentLocation();
	FVector Direction = (NextPos - CurrentPos).GetSafeNormal();

	// Calculer la rotation nécessaire (seulement Yaw pour rotation horizontale)
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

// Called every frame
void AHGOEnemyPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateEnemyRotation(DeltaTime);
}

// Called to bind functionality to input
void AHGOEnemyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AHGOEnemyPawn::AdvancePathIndex()
{
	if (PathFollowType == EPathFollowType::Loop)
	{
		// Mode boucle : revenir au début quand on arrive à la fin
		CurrentPathIndex = (CurrentPathIndex + 1) % MovementPathNodeIDs.Num();
	}
	else // PingPong
	{
		// Mode ping-pong : inverser la direction quand on atteint les extrémités
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
	// Calculer le prochain index sans le modifier
	int32 NextIndex = CurrentPathIndex;
	
	if (PathFollowType == EPathFollowType::Loop)
	{
		NextIndex = (CurrentPathIndex + 1) % MovementPathNodeIDs.Num();
	}
	else // PingPong
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

void AHGOEnemyPawn::HandleEnemyPortal()
{
	switch (PortalState)
	{
		case EEnemyPortalState::None:
			// Premier tour : construire le portail
			BuildPortal();
			break;
			
		case EEnemyPortalState::Building:
			// Deuxième tour : traverser le portail
			CrossPortal();
			break;
			
		default:
			break;
	}
}

void AHGOEnemyPawn::BuildPortal()
{
	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Building portal - 1 turn used"));
	
	PortalState = EEnemyPortalState::Building;
	
	// La construction du portail consomme le tour
	// On doit passer par ExecutingAction puis TransitioningTurn
	if (UWorld* World = GetWorld())
	{
		if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
		{
			TurnManager->RegisterActionStarted();
			
			// Lancer la rotation vers la prochaine node après le portail
			// Cela appellera automatiquement RegisterActionCompleted() à la fin
			ExecuteEnemyRotation();
		}
	}
	
	// TODO: Ajouter un effet visuel de construction de portail ici si nécessaire
}

void AHGOEnemyPawn::CrossPortal()
{
	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Crossing portal to other world"));
	
	// Changer de monde
	bInUpsideDownWorld = !bInUpsideDownWorld;
	
	// Trouver la node liée dans l'autre monde
	AHGOTacticalLevelGenerator* Generator = nullptr;
	for (TActorIterator<AHGOTacticalLevelGenerator> GeneratorItr(GetWorld()); GeneratorItr; ++GeneratorItr)
	{
		Generator = *GeneratorItr;
		break;
	}

	if (!Generator)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No level generator found for portal crossing!"));
		return;
	}

	int32 LinkedNodeID = GraphMovementComponent->GetCurrentNode()->NodeData.LinkedUpsideDownNodeID;
	
	if (LinkedNodeID < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] No linked node for this portal!"));
		return;
	}

	// Trouver la node correspondante dans l'autre monde
	UHGONodeGraphComponent* LinkedNode = nullptr;
	for (UHGONodeGraphComponent* NodeComp : Generator->NodeGraphs)
	{
		if (!NodeComp) continue;
		
		// La node liée est celle dont le NodeID correspond au LinkedUpsideDownNodeID
		// ET qui est dans le monde cible (après le changement de bInUpsideDownWorld)
		if (NodeComp->NodeData.LinkedUpsideDownNodeID == LinkedNodeID && 
			NodeComp->NodeData.bIsUpsideDownNode == bInUpsideDownWorld)
		{
			LinkedNode = NodeComp;
			break;
		}
	}

	if (!LinkedNode)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] Could not find linked node %d!"), LinkedNodeID);
		return;
	}

	// Téléporter sur la node liée (même position physique)
	GraphMovementComponent->SetCurrentNode(LinkedNode);
	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Crossed to node %d in %s world"), 
		LinkedNodeID, bInUpsideDownWorld ? TEXT("UPSIDE-DOWN") : TEXT("NORMAL"));

	OnEnemyPassThroughPortal();
	
	// IMPORTANT: Avancer dans le path pour pointer sur la node après le portail
	// Car la node actuelle (LinkedNode) est au même endroit que la précédente
	AdvancePathIndex();
	
	// Réinitialiser l'état du portail
	PortalState = EEnemyPortalState::None;
	
	// Maintenant, déplacer vers la VRAIE prochaine node (mouvement visuel)
	int32 NextNodeID = GetNextNodeID();
	
	UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Moving to next node after portal: %d"), NextNodeID);
	
	// Démarrer le mouvement vers cette prochaine node
	if (GraphMovementComponent->TryMoveToNodeID(NextNodeID))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Successfully started move to node %d after portal crossing"), NextNodeID);
		AdvancePathIndex();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyPawn] Failed to move to node %d after portal"), NextNodeID);
		
		// Si le mouvement échoue, on termine quand même le tour
		if (UWorld* World = GetWorld())
		{
			if (UHGOTacticalTurnManager* TurnManager = World->GetSubsystem<UHGOTacticalTurnManager>())
			{
				TurnManager->RegisterActionStarted();
				TurnManager->RegisterActionCompleted();
			}
		}
	}
}

void AHGOEnemyPawn::UpdateVisibilityForWorld(bool bPlayerInUpsideDownWorld)
{
	OnEnemyPassThroughPortal();
	// Afficher l'ennemi seulement si il est dans le même monde que le joueur
	bool bShouldBeVisible = (bInUpsideDownWorld == bPlayerInUpsideDownWorld);
	
	EnemyMeshComponent->SetVisibility(bShouldBeVisible, true);
	
	UE_LOG(LogTemp, Log, TEXT("[EnemyPawn] Visibility updated: %s (Player in %s, Enemy in %s)"),
		bShouldBeVisible ? TEXT("VISIBLE") : TEXT("HIDDEN"),
		bPlayerInUpsideDownWorld ? TEXT("UPSIDE-DOWN") : TEXT("NORMAL"),
		bInUpsideDownWorld ? TEXT("UPSIDE-DOWN") : TEXT("NORMAL"));
}

bool AHGOEnemyPawn::OnEnemyPassThroughPortal_Implementation()
{
	return bInUpsideDownWorld;
}

// Overlap sur la DetectionCollision : tue le joueur s'il est sur la même case physique.
// Pas de vérification de monde — l'ennemi est juste invisible dans l'autre monde,
// mais sa présence physique reste dangereuse.
void AHGOEnemyPawn::OnDetectionOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	AHGOPlayerPawn* Player = Cast<AHGOPlayerPawn>(OtherActor);
	if (!Player)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Player walked onto the same tile! Killing player (world-agnostic overlap)."));
	Player->KillPlayer();
}

bool AHGOEnemyPawn::CheckAndKillPlayer()
{
	if (!GraphMovementComponent || !GraphMovementComponent->GetCurrentNode())
	{
		return false;
	}

	// Trouver le joueur
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

	// Vérifier si le joueur est dans le même monde
	if (bInUpsideDownWorld != Player->GraphMovementComponent->bInUpsideDownWorld)
	{
		return false; // Le joueur est dans un autre monde, on ne peut pas le voir
	}

	// Vérifier si le joueur est dans le champ de vision (devant, 1 node de distance, connecté)
	if (GraphMovementComponent->IsNodeAdjacent(PlayerNode))
	{
		GraphMovementComponent->TryMoveToNodeID(PlayerNode->NodeData.NodeID); // Se tourner vers le joueur
		UE_LOG(LogTemp, Warning, TEXT("[EnemyPawn] Player detected in vision! Killing player..."));
		
		// Tuer le joueur directement
		Player->KillPlayer();
		
		
		return true;
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

	// Trouver le générateur pour parcourir le graphe
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

	// Sauvegarder la node actuelle comme point de départ
	UHGONodeGraphComponent* StartNode = GraphMovementComponent->GetCurrentNode();
	LastPatrolNodeID = StartNode->NodeData.NodeID;
	
	// Parcourir dans la direction jusqu'à trouver la dernière node connectée
	UHGONodeGraphComponent* CurrentCheck = StartNode;
	UHGONodeGraphComponent* LastValidNode = StartNode;
	PushPathNodeIDs.Empty();
	PushPathNodeIDs.Add(StartNode->NodeData.NodeID);

	// Parcourir jusqu'à 20 nodes max
	for (int32 i = 0; i < 20; ++i)
	{
		UHGONodeGraphComponent* NextNode = CurrentCheck->GetNodeInDirection(Direction);
		
		if (!NextNode)
		{
			// Dead-end trouvé
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
				FString::Printf(TEXT("[Push] Dead-end after %d nodes"), PushPathNodeIDs.Num() - 1));
			break;
		}

		// Node connectée trouvée
		LastValidNode = NextNode;
		PushPathNodeIDs.Add(NextNode->NodeData.NodeID);
		CurrentCheck = NextNode;
		
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
			FString::Printf(TEXT("[Push] Found connected node: %d"), NextNode->NodeData.NodeID));
	}

	// Vérifier si on peut pousser (au moins 1 node de déplacement)
	if (PushPathNodeIDs.Num() <= 1)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
			TEXT("[Push] No node to push to - ability consumed but enemy doesn't move"));
		return;
	}

	// Démarrer le push
	bBeingPushed = true;
	
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
		FString::Printf(TEXT("[Push] Starting push - will move %d nodes"), PushPathNodeIDs.Num() - 1));

	// Démarrer le premier mouvement
	int32 NextNodeID = PushPathNodeIDs[1]; // Première node après la position actuelle
	
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

