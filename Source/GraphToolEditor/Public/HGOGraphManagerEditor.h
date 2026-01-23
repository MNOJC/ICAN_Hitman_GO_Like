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
	
	//BASE CLASS FOR GRAPH MANAGEMENT
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Graph")
	TSubclassOf<AHGOEdgeEditor> EdgeClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Graph")
	TSubclassOf<AHGONodeEditor> NodeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph")
	bool bEditingUpsideDownGraph = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph")
	float GridSpacing = 200.0f;
	
	//FUNCTIONS CALLED BY THE EDITOR WIDGETS CLASS
	UFUNCTION(BlueprintCallable, Category = "Graph")
	void CreateConnectionFromSelection();

	
	UFUNCTION(BlueprintCallable, Category = "Graph")
	AHGONodeEditor* CreateNewNode(FVector SpawnLocation = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void SaveGraphDataAsset(UHGOTacticalLevelData* GraphDataAsset);

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void SwitchEditingWorldType(bool bIsEditingUpsideDown);

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void OnGridSpacingChanged(float NewGridSpacing);

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void OnGridRefreshed();

private:

	//HELPER FUNCTIONS
	void CreateEdgeBetweenNodes(AHGONodeEditor* Source, AHGONodeEditor* Target);
	int32 GetNextEdgeID();
	int32 GetNextNodeID();
	float ComputeCurrentGridSpacing() const;
	ENodeDirection CalculateDirection(FVector SourcePos, FVector TargetPos);
	TArray<AHGONodeEditor*> GetSelectedNodesInEditor();
};