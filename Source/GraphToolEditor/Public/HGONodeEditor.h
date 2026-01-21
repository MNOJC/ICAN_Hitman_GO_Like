// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "HGOEdgeEditor.h"
#include "Structures/GraphDataStructures.h"
#include "HGONodeEditor.generated.h"

UCLASS()
class GRAPHTOOLEDITOR_API AHGONodeEditor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHGONodeEditor();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Node")
	USceneComponent* Root;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Node")
	UStaticMeshComponent* NodeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Node")
	FNodeData NodeData;

	virtual void Destroyed() override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void DeleteConnectedEdges();
	virtual void PostEditMove(bool bFinished) override;
};
