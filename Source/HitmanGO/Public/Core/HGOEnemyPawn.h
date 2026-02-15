// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HGOGraphMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "HGOEnemyPawn.generated.h"

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
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	TArray<int32> MovementPathNodeIDs;

	// Execute the enemy's movement to the next node in path
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ExecuteEnemyMove();

private:
	int32 CurrentPathIndex = 0;

	void InitEnemyPosition();

};
