// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Graph/HGONodeGraphComponent.h"
#include "Graph/HGOEdgeGraphComponent.h"
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

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	bool bIsMoving;
	UHGONodeGraphComponent* TargetNode;
	float MovementProgress;

	void UpdateMovement(float DeltaTime);
	void UpdateGrabFeedback(float DeltaTime);

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
