// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Components/InstancedStaticMeshComponent.h" // debug copy cones
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline")
	USplineComponent* SeatSpline;

	// forward direction of seats (local)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawning")
	FVector LocalForwardDirection;


	// collums
	UPROPERTY(EditAnywhere, Category = "Spawning|Layout", meta = (ClampMin = "1.0"))
	float ColumnSpacing;

	// rows
	UPROPERTY(EditAnywhere, Category = "Spawning|Layout", meta = (ClampMin = "1.0"))
	float RowSpacing;

	// row's z offset
	UPROPERTY(EditAnywhere, Category = "Spawning|Layout", meta = (ClampMin = "0.0"))
	float RowHeightOffset;

	//debug cones ISM
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawning|Debug")
	UInstancedStaticMeshComponent* DebugSeatGridISM;

	// debug cone
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawning|Debug")
	UStaticMeshComponent* DebugCone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Debug")
	bool bShowDebugCone;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
