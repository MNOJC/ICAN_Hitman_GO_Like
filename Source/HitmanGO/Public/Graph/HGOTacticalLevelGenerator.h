// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HGOEdgeGraphComponent.h"
#include "HGONodeGraphComponent.h"
#include "GameFramework/Actor.h"
#include "LevelData/HGOTacticalLevelData.h"
#include "HGOTacticalLevelGenerator.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBoardFlipAnimCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSwitchWorldAnimCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGraphAnimationCompleted);

USTRUCT()
struct FNodeAnimationData
{
	GENERATED_BODY()

	UHGONodeGraphComponent* Node = nullptr;
	TArray<UHGOEdgeGraphComponent*> ConnectedEdges;
	int32 DistanceFromStart = 0;
	FVector TargetScale = FVector::OneVector;
	float CurrentAnimTime = 0.0f;
};

UENUM(BlueprintType)
enum class EAnimationState : uint8
{
	None,
	HidingGraph,
	WaitingForBoardFlip,
	ShowingGraph
};

UCLASS()
class HITMANGO_API AHGOTacticalLevelGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHGOTacticalLevelGenerator();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Level Data")
	UHGOTacticalLevelData* LevelData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Level Data")
	TSubclassOf<UHGONodeGraphComponent> NodeGraphClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Level Data")
	TSubclassOf<UHGOEdgeGraphComponent> EdgeGraphClass;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Events")
	FOnBoardFlipAnimCompleted OnBoardFlipAnimCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSwitchWorldAnimCompleted OnSwitchWorldAnimCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnGraphAnimationCompleted OnGraphAnimationCompleted;

	UPROPERTY()
	TArray<UHGONodeGraphComponent*> NodeGraphs;

	UPROPERTY()
	TArray<UHGOEdgeGraphComponent*> EdgeGraphs;

	// Animation settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float NodeScaleDuration = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float DelayBetweenLayers = 0.2f;

	void GenerateVisualGraph();
	void ClearVisualGraph();

	// Appelé par le GameMode pour démarrer la séquence de switch
	UFUNCTION(BlueprintCallable, Category = "Level Generator")
	void StartWorldSwitchSequence();

	// Appelé par le Blueprint après l'animation de board flip
	UFUNCTION(BlueprintCallable, Category = "Level Generator")
	void OnBoardFlipAnimationComplete();

	UFUNCTION(BlueprintCallable, Category = "Level Generator")
	void OnGraphAnimationComplete();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Animation state
	TArray<TArray<FNodeAnimationData>> AnimationLayers;
	int32 CurrentAnimLayer = 0;
	bool bIsAnimating = false;
	bool bReverseAnimation = false;
	float LayerTimer = 0.0f;

	EAnimationState CurrentAnimState = EAnimationState::None;

	void BuildAnimationLayers();
	void UpdateGraphAnimation(float DeltaTime);
	void AnimateLayer(TArray<FNodeAnimationData>& Layer, float DeltaTime);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	ENodeDirection GetOppositeDirection(ENodeDirection Direction);

	UFUNCTION(BlueprintCallable, Category = "Level Generator")
	void PlayGraphAnimation(bool bReversed);
};