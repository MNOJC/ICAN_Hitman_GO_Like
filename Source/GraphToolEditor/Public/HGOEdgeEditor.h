// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Structures/GraphDataStructures.h"
#include "HGOEdgeEditor.generated.h"

UCLASS()
class GRAPHTOOLEDITOR_API AHGOEdgeEditor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHGOEdgeEditor();

	
	//BASE COMPONENTS FOR EDGE VISUALIZATION
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Edge")
	USceneComponent* Root;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Edge")
	UStaticMeshComponent* EdgeMesh;


	//USER CAN EDIT THIS DATA IN THE EDITOR 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Edge")
	FEdgeData EdgeData;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	

};
