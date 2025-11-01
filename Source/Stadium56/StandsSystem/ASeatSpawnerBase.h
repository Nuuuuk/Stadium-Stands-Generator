// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ASeatSpawnerBase.generated.h"

UCLASS()
class STADIUM56_API AASeatSpawnerBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AASeatSpawnerBase();
	
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Spline¡£editable in child bp
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "_Parm_|Spline")
	USplineComponent* SeatSpline;

	// forward direction of seats (local)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "_Parm_")
	FVector LocalForwardDirection;


	// collums
	UPROPERTY(EditAnywhere, Category = "_Parm_|Layout", meta = (ClampMin = "1.0"))
	float ColumnSpacing;

	// rows
	UPROPERTY(EditAnywhere, Category = "_Parm_|Layout", meta = (ClampMin = "1.0"))
	float RowSpacing;

	// row's z offset
	UPROPERTY(VisibleInstanceOnly, Category = "_Parm_|Layout", meta = (ClampMin = "0.0"))
	float RowHeightOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "_Parm_|Debug")
	UHierarchicalInstancedStaticMeshComponent* SeatGridHISM;

	// debug cone
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "_Parm_|Debug")
	UStaticMeshComponent* DebugCone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "_Parm_|Debug")
	bool bShowDebugCone;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
