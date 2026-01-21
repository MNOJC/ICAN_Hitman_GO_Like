#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Structures/GraphDataStructures.h"
#include "HGOEdgeGraphComponent.generated.h"

UCLASS(ClassGroup=(Graph), meta=(BlueprintSpawnableComponent))
class HITMANGO_API UHGOEdgeGraphComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UHGOEdgeGraphComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Graph")
	FEdgeData EdgeData;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
