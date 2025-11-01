// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "ASeatSpawnerBase.generated.h"

class AAGlobalSeatManager;

UCLASS(meta = (PrioritizeCategories = "_Parm_"))
class STADIUM56_API AASeatSpawnerBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AASeatSpawnerBase(); 
	
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;
	FVector GetLocalForwardDirection() const { return LocalForwardDirection; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// spawner manager
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "_Parm_|Manager", meta = (DisplayPriority = "-1"))
	AAGlobalSeatManager* SeatManager;

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


	// lock 0 and clamp all pts' z>0
	void UpdateAndValidateSpline();

	// scan row intersection algorithm to calculate seat transforms
	TArray<FTransform> GenerateTransforms();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
