// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HGOGraphMovementComponent.h"
#include "Components/BoxComponent.h"
#include "FMODEvent.h"
#include "FMODBlueprintStatics.h"
#include "GameFramework/Pawn.h"
#include "Core/HGOPlayerPawn.h"
#include "HGOEnemyPawn.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPortalCreated, int32, PortalNodeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPortalCrossed, int32, PortalNodeID);

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
	
	AHGOEnemyPawn();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	USceneComponent* SceneRoot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* EnemyMeshComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UBoxComponent* DetectionCollision;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy")
	UHGOGraphMovementComponent* GraphMovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy|Detection")
	UFMODEvent* PortalCreateSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy|Detection")
	UFMODEvent* EnemyUpSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy|Detection")
	UFMODEvent* EnemyCrossPortalSound;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Portal")
	FOnPortalCreated OnPortalCreated;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Portal")
	FOnPortalCrossed OnPortalCrossed;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Path")
	TArray<int32> MovementPathNodeIDs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Path")
	EPathFollowType PathFollowType = EPathFollowType::Loop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Path")
	FRotator DefaultRotation = FRotator::ZeroRotator;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Portal")
	float PortalDiveOffsetZ = -50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Portal")
	float PortalDiveDuration = 1.0f;
	
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ExecuteEnemyMove();

	void ExecuteEnemyRotation();
	
	void HandlePortalOnArrival();
	
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void UpdateVisibilityForWorld(bool bPlayerInUpsideDownWorld);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool IsInUpsideDownWorld() const { return bInUpsideDownWorld; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy")
	bool OnEnemyPassThroughPortal();
	
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool CheckAndKillPlayer();
	
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void PushEnemy(ENodeDirection Direction);

	void StartReturnToPatrol();
	bool IsNodeInPatrol(int32 NodeID) const;

	void HandleEnemyPortal();
	
	bool bBeingPushed = false;
	bool bReturningToPatrol = false;
	bool bKillMoveInProgress = false;
	TArray<int32> PushPathNodeIDs;

private:

	int32 CurrentPathIndex = 0;
	bool bReverseDirection = false; 
	
	bool bInUpsideDownWorld = false;
	
	EEnemyPortalState PortalState = EEnemyPortalState::None;
	
	bool bJustCrossedPortal = false;
	
	bool bIsRotating = false;
	FRotator NextRotation;
	
	int32 LastPatrolNodeID = -1;

	void InitEnemyPosition();
	void UpdateEnemyRotation(float DeltaTime);
	
	void AdvancePathIndex();
	int32 GetNextNodeID();
	
	void BuildPortal();
	void CrossPortal();
	
	UFUNCTION()
	void OnDetectionOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool bIsPortalDiving = false;
	float PortalDiveElapsed = 0.0f;
	FVector PortalDiveStartLocation = FVector::ZeroVector;
	FVector PortalDiveTargetLocation = FVector::ZeroVector;

	void StartPortalDive();
	void UpdatePortalDive(float DeltaTime);

	bool bPortalVisualStateAppliedBeforeCross = false;
	
	
};
