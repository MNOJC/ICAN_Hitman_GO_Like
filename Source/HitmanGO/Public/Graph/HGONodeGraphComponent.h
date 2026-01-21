// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Structures/GraphDataStructures.h"
#include "HGONodeGraphComponent.generated.h"

UCLASS(ClassGroup=(Graph), meta=(BlueprintSpawnableComponent))
class HITMANGO_API UHGONodeGraphComponent : public UStaticMeshComponent
{
	GENERATED_BODY()
	
public:	

	UHGONodeGraphComponent();

	UPROPERTY(BlueprintReadWrite, Category = "Graph")
	FNodeData NodeData;

	UPROPERTY()
	TMap<ENodeDirection, UHGONodeGraphComponent*> ConnectedNodes;

	UFUNCTION(BlueprintCallable, Category = "Graph")
	UHGONodeGraphComponent* GetNodeInDirection(ENodeDirection Direction);

	UFUNCTION(BlueprintCallable, Category = "Graph")
	bool CanMoveInDirection(ENodeDirection Direction);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

};
