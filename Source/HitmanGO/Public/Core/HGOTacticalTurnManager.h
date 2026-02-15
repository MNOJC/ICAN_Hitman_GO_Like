// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HGOTacticalTurnManager.generated.h"

UENUM(BlueprintType)
enum class ETurnState : uint8
{
	PlayerTurn,
	EnemyTurn
};

UENUM(BlueprintType)
enum class ETurnPhase : uint8
{
	Idle,
	WaitingForInput,
	ExecutingAction,
	TransitioningTurn
};

// Delegate fired when turn changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnChanged, ETurnState, NewTurnState);

// Delegate fired when phase changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged, ETurnPhase, NewPhase);

/**
 * Simple, scalable turn-based system for Hitman GO style gameplay
 * 
 * Flow:
 * PlayerTurn -> WaitingForInput -> Player swipes -> ExecutingAction (movement) -> 
 * TransitioningTurn -> EnemyTurn -> ExecutingAction (enemy AI/debug) -> TransitioningTurn -> PlayerTurn
 */
UCLASS()
class HITMANGO_API UHGOTacticalTurnManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	
	// Subsystem overrides
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// Tick function
	void Tick(float DeltaTime);
	bool IsTickable() const { return true; }

	// Public query functions
	UFUNCTION(BlueprintCallable, Category = "Turn System")
	bool IsPlayerTurn() const { return CurrentTurnState == ETurnState::PlayerTurn; }
	
	UFUNCTION(BlueprintCallable, Category = "Turn System")
	bool CanPlayerAct() const { return CurrentTurnState == ETurnState::PlayerTurn && CurrentPhase == ETurnPhase::WaitingForInput; }

	UFUNCTION(BlueprintCallable, Category = "Turn System")
	ETurnState GetCurrentTurnState() const { return CurrentTurnState; }

	UFUNCTION(BlueprintCallable, Category = "Turn System")
	ETurnPhase GetCurrentPhase() const { return CurrentPhase; }

	// Action registration - called by movement component
	UFUNCTION(BlueprintCallable, Category = "Turn System")
	void RegisterActionStarted();
	
	UFUNCTION(BlueprintCallable, Category = "Turn System")
	void RegisterActionCompleted();

	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "Turn System")
	FOnTurnChanged OnTurnChanged;

	UPROPERTY(BlueprintAssignable, Category = "Turn System")
	FOnPhaseChanged OnPhaseChanged;

private:
	
	// Current state
	ETurnState CurrentTurnState = ETurnState::PlayerTurn;
	ETurnPhase CurrentPhase = ETurnPhase::Idle;

	// Enemy turn simulation
	float EnemyTurnTimer = 0.f;
	float EnemyTurnDuration = 2.0f; // Debug: simulates enemy "thinking" time

	// State machine helpers
	void ChangePhase(ETurnPhase NewPhase);
	void ChangeTurn(ETurnState NewTurnState);
	
	// Turn logic
	void TickPlayerTurn(float DeltaTime);
	void TickEnemyTurn(float DeltaTime);
	
	void StartPlayerTurn();
	void StartEnemyTurn();
};
