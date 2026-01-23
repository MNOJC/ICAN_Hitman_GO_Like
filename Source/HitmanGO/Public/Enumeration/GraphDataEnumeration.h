// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GraphDataEnumeration.generated.h"

UENUM(BlueprintType)
enum class ENodeType : uint8
{
	Normal		UMETA(DisplayName = "Normal"),
	Start		UMETA(DisplayName = "Start"),
	Goal		UMETA(DisplayName = "Goal"),
	Blocked		UMETA(DisplayName = "Blocked"),
	UpsideDown		UMETA(DisplayName = "UpsideDown"),
};

UENUM(BlueprintType)
enum class ENodeDirection : uint8
{
	North       UMETA(DisplayName = "North"),
	South       UMETA(DisplayName = "South"),
	East        UMETA(DisplayName = "East"),
	West        UMETA(DisplayName = "West"),
	None        UMETA(DisplayName = "None")
};