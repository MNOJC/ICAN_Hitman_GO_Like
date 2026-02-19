// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HGOGraphMovementComponent.h"
#include "GameFramework/Pawn.h"
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

	// Execute the enemy's movement to the next node in path
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ExecuteEnemyMove();

	void ExecuteEnemyRotation();

	// Update visibility based on world state
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void UpdateVisibilityForWorld(bool bPlayerInUpsideDownWorld);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool IsInUpsideDownWorld() const { return bInUpsideDownWorld; }

private:
	// Path navigation
	int32 CurrentPathIndex = 0;
	bool bReverseDirection = false; // Pour le mode PingPong

	// World state
	bool bInUpsideDownWorld = false;

	// Portal state
	EEnemyPortalState PortalState = EEnemyPortalState::None;

	// Rotation
	bool bIsRotating = false;
	FRotator NextRotation;

	void InitEnemyPosition();
	void UpdateEnemyRotation(float DeltaTime);
	
	// Path navigation helpers
	void AdvancePathIndex();
	int32 GetNextNodeID();
	
	// Portal handling
	void HandleEnemyPortal();
	void BuildPortal();
	void CrossPortal();
};
