// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HGOGraphMovementComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Core/HGOPlayerPawn.h"
#include "HGOEnemyPawn.generated.h"

UENUM(BlueprintType)
enum class EPathFollowType : uint8
{
	Loop UMETA(DisplayName = "Loop"),
	PingPong UMETA(DisplayName = "PingPong")
};

UENUM(BlueprintType)
enum class EEnemyPortalState : uint8
{
	None UMETA(DisplayName = "None"),
	Building UMETA(DisplayName = "Building Portal"),
	ReadyToCross UMETA(DisplayName = "Ready To Cross")
};

UCLASS()
class HITMANGO_API AHGOEnemyPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AHGOEnemyPawn();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	USceneComponent* SceneRoot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* EnemyMeshComponent;

	// Collision box for detecting player overlap (works across worlds - enemy is just invisible)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UBoxComponent* DetectionCollision;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy")
	UHGOGraphMovementComponent* GraphMovementComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// Path configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Path")
	TArray<int32> MovementPathNodeIDs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Path")
	EPathFollowType PathFollowType = EPathFollowType::Loop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Path")
	FRotator DefaultRotation = FRotator::ZeroRotator;

	// Execute the enemy's movement to the next node in path
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ExecuteEnemyMove();

	void ExecuteEnemyRotation();

	// Appelé par GraphMovementComponent quand l'ennemi arrive sur une node portail
	void HandlePortalOnArrival();

	// Update visibility based on world state
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void UpdateVisibilityForWorld(bool bPlayerInUpsideDownWorld);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool IsInUpsideDownWorld() const { return bInUpsideDownWorld; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy")
	bool OnEnemyPassThroughPortal();

	// Check if player is in vision and kill if true
	
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool CheckAndKillPlayer();

	// Push enemy in a direction (called by player ability)
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void PushEnemy(ENodeDirection Direction);

	void StartReturnToPatrol();
	bool IsNodeInPatrol(int32 NodeID) const;

	void HandleEnemyPortal();

	// Push state (accessed by GraphMovementComponent)
	bool bBeingPushed = false;
	bool bReturningToPatrol = false;
	TArray<int32> PushPathNodeIDs;

private:
	// Path navigation
	int32 CurrentPathIndex = 0;
	bool bReverseDirection = false; // Pour le mode PingPong

	// World state
	bool bInUpsideDownWorld = false;

	// Portal state
	EEnemyPortalState PortalState = EEnemyPortalState::None;

	// Empêche le re-trigger de construction de portail quand l'ennemi arrive
	// sur la node liée (qui est aussi de type EnemyPortal) après avoir traversé
	bool bJustCrossedPortal = false;

	// Rotation
	bool bIsRotating = false;
	FRotator NextRotation;

	// Push state (private)
	int32 LastPatrolNodeID = -1;   // Dernière node de la patrol avant le push

	void InitEnemyPosition();
	void UpdateEnemyRotation(float DeltaTime);
	
	// Path navigation helpers
	void AdvancePathIndex();
	int32 GetNextNodeID();
	
	// Portal handling
	
	void BuildPortal();
	void CrossPortal();

	// Overlap callback for detection collision (kills player on overlap, regardless of world)
	UFUNCTION()
	void OnDetectionOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
	
	
	
};
