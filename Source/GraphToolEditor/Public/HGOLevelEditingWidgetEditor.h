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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graph Data")
	UHGOTacticalLevelData* GraphDataAsset;

	UFUNCTION(BlueprintCallable, Category="Graph Tool")
	void OnConnectNodesButtonClicked();
	
	UFUNCTION(BlueprintCallable, Category="Graph Tool")
	void OnCreateNodeButtonClicked();
	
	UFUNCTION(BlueprintCallable, Category="Graph Tool")
	void OnSaveGraphButtonClicked();
};
