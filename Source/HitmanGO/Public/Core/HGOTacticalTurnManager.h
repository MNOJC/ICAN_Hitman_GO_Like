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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnChanged, ETurnState, NewTurnState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged, ETurnPhase, NewPhase);

UCLASS()
class HITMANGO_API UHGOTacticalTurnManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// Tick function
	bool Tick(float DeltaTime);
	
	UFUNCTION(BlueprintCallable, Category = "Turn System")
	bool IsPlayerTurn() const { return CurrentTurnState == ETurnState::PlayerTurn; }
	
	UFUNCTION(BlueprintCallable, Category = "Turn System")
	bool CanPlayerAct() const { return CurrentTurnState == ETurnState::PlayerTurn && CurrentPhase == ETurnPhase::WaitingForInput; }

	UFUNCTION(BlueprintCallable, Category = "Turn System")
	ETurnState GetCurrentTurnState() const { return CurrentTurnState; }

	UFUNCTION(BlueprintCallable, Category = "Turn System")
	ETurnPhase GetCurrentPhase() const { return CurrentPhase; }
	
	UFUNCTION(BlueprintCallable, Category = "Turn System")
	void RegisterActionStarted();
	
	UFUNCTION(BlueprintCallable, Category = "Turn System")
	void RegisterActionCompleted();
	
	UFUNCTION(BlueprintCallable, Category = "Turn System")
	void StopGame();

	UFUNCTION(BlueprintCallable, Category = "Turn System")
	bool IsGameOver() const { return bGameOver; }

	UPROPERTY(BlueprintAssignable, Category = "Turn System")
	FOnTurnChanged OnTurnChanged;

	UPROPERTY(BlueprintAssignable, Category = "Turn System")
	FOnPhaseChanged OnPhaseChanged;

private:
	
	ETurnState CurrentTurnState = ETurnState::PlayerTurn;
	ETurnPhase CurrentPhase = ETurnPhase::Idle;
	
	bool bGameOver = false;
	
	float EnemyTurnTimer = 0.f;
	float EnemyTurnCooldownTimer = 0.f;
	
	void ChangePhase(ETurnPhase NewPhase);
	void ChangeTurn(ETurnState NewTurnState);
	
	void TickPlayerTurn(float DeltaTime);
	void TickEnemyTurn(float DeltaTime);
	
	void StartPlayerTurn();
	void StartEnemyTurn();

	FTSTicker::FDelegateHandle TickHandle;
};
