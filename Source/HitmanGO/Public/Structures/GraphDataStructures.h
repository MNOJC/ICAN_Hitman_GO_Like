// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enumeration/GraphDataEnumeration.h"
#include "GraphDataStructures.generated.h" 

USTRUCT(BlueprintType)
struct FNodeData 
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NodeID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Position = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ENodeType NodeType = ENodeType::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsUpsideDownNode;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LinkedUpsideDownNodeID = 0;
	
};


USTRUCT(BlueprintType)
struct FEdgeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EdgeID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SourceNodeID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TargetNodeID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsBidirectional = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ENodeDirection Direction = ENodeDirection::None;
	
};
		