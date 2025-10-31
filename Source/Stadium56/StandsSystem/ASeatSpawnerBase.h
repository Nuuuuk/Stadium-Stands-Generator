// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
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

	/**
	 * Spline¡£editable in child bp
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline")
	USplineComponent* SeatSpline;

	/**
	 * forward direction of seats (local)¡£
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawning")
	FVector LocalForwardDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawning")
	UStaticMeshComponent* DebugCone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	bool bShowDebugCone;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
