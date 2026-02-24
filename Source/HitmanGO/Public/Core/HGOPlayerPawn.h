// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "EngineUtils.h"
#include "HGOGraphMovementComponent.h"
#include "HGOPlayerPawn.generated.h"

struct FKey;

// Delegate pour la mort du joueur
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDeath);

UCLASS()
class HITMANGO_API AHGOPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AHGOPlayerPawn();

	// Delegate appelé quand le joueur meurt
	UPROPERTY(BlueprintAssignable, Category = "Player|Events")
	FOnPlayerDeath OnPlayerDeath;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UHGOGraphMovementComponent* GraphMovementComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	USceneComponent* SceneRoot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* PlayerMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UBoxComponent* CollisionSwipeComponent;

	

	UFUNCTION()
	void OnPawnClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

	UFUNCTION()
	void OnPawnReleased(UPrimitiveComponent* TouchedComponent, FKey ButtonReleased);
	

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void InitPawnPosition();

	// Fonction pour tuer le joueur
	UFUNCTION(BlueprintCallable, Category = "Player")
	void KillPlayer();

};
