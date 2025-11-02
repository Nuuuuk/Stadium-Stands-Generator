// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "ACrowdVolume.generated.h"

UCLASS(meta = (PrioritizeCategories = "Parm"))
class STADIUM56_API AACrowdVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AACrowdVolume();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Parm")
	FBox GetQueryBox() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// visualize box volume for querying crowd positions
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parm")
	UBoxComponent* QueryBox;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parm", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CrowdDensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parm")
	int32 RandomSeed;
};
