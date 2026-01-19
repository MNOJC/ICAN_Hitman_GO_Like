// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Structures/GraphDataStructures.h"
#include "HGOTacticalLevelData.generated.h"

/**
 * 
 */
UCLASS()
class HITMANGO_API UHGOTacticalLevelData : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Graph")
	TArray<FNodeData> Nodes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Graph")
	TArray<FEdgeData> Edges;
	
};
