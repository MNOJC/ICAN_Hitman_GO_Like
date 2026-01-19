// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HGOEdgeGraph.h"
#include "HGONodeGraph.h"
#include "GameFramework/Actor.h"
#include "LevelData/HGOTacticalLevelData.h"
#include "HGOTacticalLevelGenerator.generated.h"

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
	TSubclassOf<AHGONodeGraph> NodeGraphClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Level Data")
	TSubclassOf<AHGOEdgeGraph> EdgeGraphClass;

	UPROPERTY()
	TArray<AHGONodeGraph*> NodeGraphs;

	UPROPERTY()
	TArray<AHGOEdgeGraph*> EdgeGraphs;

	void GenerateVisualGraph();
	void ClearVisualGraph();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	ENodeDirection GetOppositeDirection(ENodeDirection Direction);

};
