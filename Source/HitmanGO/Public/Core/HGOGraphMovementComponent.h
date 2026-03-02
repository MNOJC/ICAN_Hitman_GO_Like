// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Graph/HGONodeGraphComponent.h"
#include "Graph/HGOEdgeGraphComponent.h"
#include "Core/HGOGameMode.h"
#include "HGOGraphMovementComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovementCompleted, int32, NewNode);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HITMANGO_API UHGOGraphMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UHGOGraphMovementComponent();

	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	UHGONodeGraphComponent* CurrentNode;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MovementSpeed = 700.0f;

	UPROPERTY(BlueprintReadWrite, Category = "State")
	bool bInUpsideDownWorld = false;

	UPROPERTY(BlueprintAssignable, Category = "Movement")
	FOnMovementCompleted OnMovementCompleted;

	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool TryMoveInDirection(ENodeDirection Direction);
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetCurrentNode(UHGONodeGraphComponent* NewNode);
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SwitchWorldGraph();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	UHGONodeGraphComponent* GetCurrentNode() const { return CurrentNode; }

	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool TryMoveToNodeID(int32 TargetNodeID);
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool IsNodeInFrontDirection(UHGONodeGraphComponent* TargetedNode) const;

	// Check if a target node is aligned in a straight direction (any distance, must be connected)
	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool IsNodeInAlignedDirection(UHGONodeGraphComponent* TargetedNode, ENodeDirection& OutDirection) const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	bool bIsMoving;
	UHGONodeGraphComponent* TargetNode;
	float MovementProgress;
	
	void UpdateMovement(float DeltaTime);
	void UpdateGrabFeedback(float DeltaTime);
	void HideShowGraph(TArray<UHGOEdgeGraphComponent*> EdgesToProcess, TArray<UHGONodeGraphComponent*> NodesToProcess,bool bHide);

	// Turn system integration
	void NotifyMovementStarted();
	void NotifyMovementCompleted();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
