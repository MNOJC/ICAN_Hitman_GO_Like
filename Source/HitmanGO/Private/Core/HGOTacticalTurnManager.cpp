// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/HGOTacticalTurnManager.h"

void UHGOTacticalTurnManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(LogTemp, Log, TEXT("[TurnManager] Initialized"));
	
	// Start with player turn
	StartPlayerTurn();
}

void UHGOTacticalTurnManager::Deinitialize()
{
	Super::Deinitialize();
	
	UE_LOG(LogTemp, Log, TEXT("[TurnManager] Deinitialized"));
}

void UHGOTacticalTurnManager::Tick(float DeltaTime)
{
	UE_LOG(LogTemp, Verbose, TEXT("[TurnManager] Tick - Current Turn: %s | Phase: %s"), *UEnum::GetValueAsString(CurrentTurnState), *UEnum::GetValueAsString(CurrentPhase));	
	switch (CurrentTurnState)
	{
		case ETurnState::PlayerTurn:
			TickPlayerTurn(DeltaTime);
			break;
			
		case ETurnState::EnemyTurn:
			TickEnemyTurn(DeltaTime);
			break;
	}
}

void UHGOTacticalTurnManager::RegisterActionStarted()
{
	if (CurrentPhase == ETurnPhase::WaitingForInput)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnManager] Action started"));
		ChangePhase(ETurnPhase::ExecutingAction);
	}
}

void UHGOTacticalTurnManager::RegisterActionCompleted()
{
	if (CurrentPhase == ETurnPhase::ExecutingAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnManager] Action completed"));
		ChangePhase(ETurnPhase::TransitioningTurn);
	}
}

void UHGOTacticalTurnManager::ChangePhase(ETurnPhase NewPhase)
{
	if (CurrentPhase == NewPhase)
		return;
		
	CurrentPhase = NewPhase;
	OnPhaseChanged.Broadcast(CurrentPhase);
	
	UE_LOG(LogTemp, Warning, TEXT("[TurnManager] Phase changed to: %d"), CurrentPhase);

	if (CurrentPhase == ETurnPhase::TransitioningTurn)
	{
		ChangeTurn(CurrentTurnState == ETurnState::PlayerTurn ? ETurnState::EnemyTurn : ETurnState::PlayerTurn);
	}
}

void UHGOTacticalTurnManager::ChangeTurn(ETurnState NewTurnState)
{
	if (CurrentTurnState == NewTurnState)
		return;
		
	CurrentTurnState = NewTurnState;
	OnTurnChanged.Broadcast(CurrentTurnState);
	
	UE_LOG(LogTemp, Warning, TEXT("[TurnManager] Turn changed to: %s"), 
		CurrentTurnState == ETurnState::PlayerTurn ? TEXT("PLAYER") : TEXT("ENEMY"));
	
	// Start the new turn
	if (CurrentTurnState == ETurnState::PlayerTurn)
	{
		StartPlayerTurn();
	}
	else
	{
		StartEnemyTurn();
	}
}

void UHGOTacticalTurnManager::TickPlayerTurn(float DeltaTime)
{
	GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Green, FString::Printf(TEXT("[TurnManager] %s | Phase: %s"), *UEnum::GetValueAsString(CurrentTurnState), *UEnum::GetValueAsString(CurrentPhase)));

	// If action completed, transition to enemy turn
	if (CurrentPhase == ETurnPhase::TransitioningTurn)
	{
		ChangeTurn(ETurnState::EnemyTurn);
	}
}

void UHGOTacticalTurnManager::TickEnemyTurn(float DeltaTime)
{
	switch (CurrentPhase)
	{
		case ETurnPhase::WaitingForInput:
			// Simulate enemy "thinking" time
			EnemyTurnTimer += DeltaTime;
			
			if (EnemyTurnTimer >= 0.5f) // Short delay before enemy acts
			{
				UE_LOG(LogTemp, Warning, TEXT("[TurnManager] Enemy starting action"));
				ChangePhase(ETurnPhase::ExecutingAction);
				EnemyTurnTimer = 0.f;
			}
			break;
			
		case ETurnPhase::ExecutingAction:
			// Simulate enemy action duration
			EnemyTurnTimer += DeltaTime;
			
			if (EnemyTurnTimer >= EnemyTurnDuration)
			{
				UE_LOG(LogTemp, Warning, TEXT("[TurnManager] Enemy action complete"));
				ChangePhase(ETurnPhase::TransitioningTurn);
				EnemyTurnTimer = 0.f;
			}
			break;
			
		case ETurnPhase::TransitioningTurn:
			// Transition back to player turn
			ChangeTurn(ETurnState::PlayerTurn);
			break;
			
		default:
			break;
	}
}

void UHGOTacticalTurnManager::StartPlayerTurn()
{
	UE_LOG(LogTemp, Warning, TEXT("[TurnManager] === PLAYER TURN START ==="));
	ChangePhase(ETurnPhase::WaitingForInput);
}

void UHGOTacticalTurnManager::StartEnemyTurn()
{
	UE_LOG(LogTemp, Warning, TEXT("[TurnManager] === ENEMY TURN START ==="));
	EnemyTurnTimer = 0.f;
	ChangePhase(ETurnPhase::WaitingForInput);
	
	// TODO: When you add enemy AI, broadcast a delegate here that enemies can listen to
	// For now, the tick function will handle the debug simulation
}
