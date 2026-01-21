// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HGONodeEditor.h"
#include "HGOEdgeEditor.h"
#include "LevelData/HGOTacticalLevelData.h"
#include "HGOGraphManagerEditor.generated.h"

UCLASS()
class GRAPHTOOLEDITOR_API AHGOGraphManagerEditor : public AActor
{
	GENERATED_BODY()
    
public:    
	AHGOGraphManagerEditor();

protected:
	virtual void BeginPlay() override;

public:    
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Graph")
	TSubclassOf<AHGOEdgeEditor> EdgeClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Graph")
	TSubclassOf<AHGONodeEditor> NodeClass;
	
	UFUNCTION(BlueprintCallable, Category = "Graph")
	void CreateConnectionFromSelection();

	UFUNCTION(BlueprintCallable, Category = "Graph")
	AHGONodeEditor* CreateNewNode(FVector SpawnLocation = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void SaveGraphDataAsset(UHGOTacticalLevelData* GraphDataAsset);

private:
	void CreateEdgeBetweenNodes(AHGONodeEditor* Source, AHGONodeEditor* Target);
	int32 GetNextEdgeID();
	int32 GetNextNodeID();
	ENodeDirection CalculateDirection(FVector SourcePos, FVector TargetPos);
	TArray<AHGONodeEditor*> GetSelectedNodesInEditor();
};