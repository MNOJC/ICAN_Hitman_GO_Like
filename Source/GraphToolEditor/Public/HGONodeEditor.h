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

	//BASE COMPONENTS FOR NODE VISUALIZATION
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Node")
	USceneComponent* Root;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Node")
	UStaticMeshComponent* NodeMesh;

	//USER CAN EDIT THIS DATA IN THE EDITOR
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Node")
	FNodeData NodeData;
	
	//METHODS OVERRIDDEN TO HANDLE NODE EDITING IN THE EDITOR
	virtual void Destroyed() override;
	virtual void PostEditMove(bool bFinished) override;

	//DO WHAT THE FUNCTION SAYS
	void DeleteConnectedEdges();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	
};
