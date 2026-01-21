// Fill out your copyright notice in the Description page of Project Settings.


#include "Graph/HGOEdgeGraphComponent.h"

// Sets default values
UHGOEdgeGraphComponent::UHGOEdgeGraphComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void UHGOEdgeGraphComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UHGOEdgeGraphComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}


