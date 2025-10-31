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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/**
	 * Spline¡£editable in child bp
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline", meta = (AllowPrivateAccess = "true"))
	USplineComponent* SeatSpline;

	/**
	 * forward direction of seats (local)¡£
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (AllowPrivateAccess = "true"))
	FVector LocalForwardDirection;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
