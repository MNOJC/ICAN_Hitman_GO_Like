// Fill out your copyright notice in the Description page of Project Settings.


#include "HGOLevelEditingWidgetEditor.h"
#include "Kismet/GameplayStatics.h"


void UHGOLevelEditingWidgetEditor::OnConnectNodesButtonClicked()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHGOGraphManagerEditor::StaticClass(), FoundActors);
    
	if (FoundActors.Num() > 0)
	{
		AHGOGraphManagerEditor* Manager = Cast<AHGOGraphManagerEditor>(FoundActors[0]);
		if (Manager)
		{
			Manager->CreateConnectionFromSelection();
		}
	}
}

void UHGOLevelEditingWidgetEditor::OnCreateNodeButtonClicked()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHGOGraphManagerEditor::StaticClass(), FoundActors);
    
	if (FoundActors.Num() > 0)
	{
		AHGOGraphManagerEditor* Manager = Cast<AHGOGraphManagerEditor>(FoundActors[0]);
		if (Manager)
		{
			Manager->CreateNewNode();
		}
	}
}

void UHGOLevelEditingWidgetEditor::OnSaveGraphButtonClicked()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHGOGraphManagerEditor::StaticClass(), FoundActors);
    
	if (FoundActors.Num() > 0)
	{
		AHGOGraphManagerEditor* Manager = Cast<AHGOGraphManagerEditor>(FoundActors[0]);
		if (Manager)
		{
			Manager->SaveGraphDataAsset(GraphDataAsset);
		}
	}
}

void UHGOLevelEditingWidgetEditor::OnSwitchEditingWorldType(bool bIsEditingUpsideDown)
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHGOGraphManagerEditor::StaticClass(), FoundActors);
    
	if (FoundActors.Num() > 0)
	{
		AHGOGraphManagerEditor* Manager = Cast<AHGOGraphManagerEditor>(FoundActors[0]);
		if (Manager)
		{
			Manager->SwitchEditingWorldType(bIsEditingUpsideDown);
		}
	}
}

void UHGOLevelEditingWidgetEditor::OnGridSpacingChanged(float NewGridSpacing)
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHGOGraphManagerEditor::StaticClass(), FoundActors);
    
	if (FoundActors.Num() > 0)
	{
		AHGOGraphManagerEditor* Manager = Cast<AHGOGraphManagerEditor>(FoundActors[0]);
		if (Manager)
		{
			Manager->OnGridSpacingChanged(NewGridSpacing);
		}
	}
}

void UHGOLevelEditingWidgetEditor::OnGridRefreshed()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHGOGraphManagerEditor::StaticClass(), FoundActors);
    
	if (FoundActors.Num() > 0)
	{
		AHGOGraphManagerEditor* Manager = Cast<AHGOGraphManagerEditor>(FoundActors[0]);
		if (Manager)
		{
			Manager->OnGridRefreshed();
		}
	}
}
