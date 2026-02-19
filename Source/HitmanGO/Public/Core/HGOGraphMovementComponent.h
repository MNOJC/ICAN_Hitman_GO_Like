// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Graph/HGONodeGraphComponent.h"
#include "Graph/HGOEdgeGraphComponent.h"
#include "Core/HGOGameMode.h"
#include "HGOGraphMovementComponent.generated.h"




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

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	bool bIsMoving;
	UHGONodeGraphComponent* TargetNode;
	float MovementProgress;
	bool bInUpsideDownWorld = false;

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
