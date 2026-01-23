// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "LevelData/HGOTacticalLevelData.h"
#include "HGOGraphManagerEditor.h"
#include "HGOLevelEditingWidgetEditor.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class GRAPHTOOLEDITOR_API UHGOLevelEditingWidgetEditor : public UEditorUtilityWidget
{
	GENERATED_BODY()
public:

	//DATA ASSET TO SAVE THE GRAPH DATA
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graph Data")
	UHGOTacticalLevelData* GraphDataAsset;


	//METHODS CALLED BY THE WIDGET BUTTONS
	UFUNCTION(BlueprintCallable, Category="Graph Tool")
	void OnConnectNodesButtonClicked();
	
	UFUNCTION(BlueprintCallable, Category="Graph Tool")
	void OnCreateNodeButtonClicked();
	
	UFUNCTION(BlueprintCallable, Category="Graph Tool")
	void OnSaveGraphButtonClicked();

	UFUNCTION(BlueprintCallable, Category="Graph Tool")
	void OnSwitchEditingWorldType(bool bIsEditingUpsideDown);

	UFUNCTION(BlueprintCallable, Category="Graph Tool")
	void OnGridSpacingChanged(float NewGridSpacing);

	UFUNCTION(BlueprintCallable, Category="Graph Tool")
	void OnGridRefreshed();
};
