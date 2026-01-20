// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/HGOGraphMovementComponent.h"

// Sets default values for this component's properties
UHGOGraphMovementComponent::UHGOGraphMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bIsMoving = false;
	MovementProgress = 0.0f;
}


bool UHGOGraphMovementComponent::TryMoveInDirection(ENodeDirection Direction)
{
	if (bIsMoving)
	{
		return false;
	}
	
	if (!CurrentNode)
	{
		return false;
	}
	
	AHGONodeGraph* NextNode = CurrentNode->GetNodeInDirection(Direction);

	if (!NextNode)
	{
		return false;
	}
	
	TargetNode = NextNode;
	bIsMoving = true;
	MovementProgress = 0.0f;

	return true;
}

void UHGOGraphMovementComponent::SetCurrentNode(AHGONodeGraph* NewNode)
{
	if (NewNode)
	{
		CurrentNode = NewNode;
		GetOwner()->SetActorLocation(NewNode->GetActorLocation());
		bIsMoving = false;
		MovementProgress = 0.0f;
	}
}

void UHGOGraphMovementComponent::UpdateMovement(float DeltaTime)
{
	if (!bIsMoving || !CurrentNode || !TargetNode)
		return;
	
	MovementProgress += DeltaTime * MovementSpeed / 100.0f;

	if (MovementProgress >= 1.0f)
	{
		GetOwner()->SetActorLocation(TargetNode->GetActorLocation());
		CurrentNode = TargetNode;
		TargetNode = nullptr;
		bIsMoving = false;
		MovementProgress = 0.0f;
		
	}
	else
	{
		FVector StartPos = CurrentNode->GetActorLocation();
		FVector EndPos = TargetNode->GetActorLocation();
		FVector NewPos = FMath::Lerp(StartPos, EndPos, MovementProgress);
        
		GetOwner()->SetActorLocation(NewPos);
	}
}

void UHGOGraphMovementComponent::UpdateGrabFeedback(float DeltaTime)
{
	
}

// Called when the game starts
void UHGOGraphMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UHGOGraphMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsMoving)
	{
		UpdateMovement(DeltaTime);
	}
}

