// Fill out your copyright notice in the Description page of Project Settings.


#include "Graph/HGONodeGraphComponent.h"

// Sets default values
UHGONodeGraphComponent::UHGONodeGraphComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

UHGONodeGraphComponent* UHGONodeGraphComponent::GetNodeInDirection(ENodeDirection Direction)
{
	if (UHGONodeGraphComponent** FoundNode = ConnectedNodes.Find(Direction))
	{
		return *FoundNode;
	}
	return nullptr;
}

bool UHGONodeGraphComponent::CanMoveInDirection(ENodeDirection Direction)
{
	return ConnectedNodes.Contains(Direction);
}

// Called when the game starts or when spawned
void UHGONodeGraphComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UHGONodeGraphComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

