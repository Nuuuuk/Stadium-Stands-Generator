// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AGlobalSeatManager.generated.h"

USTRUCT()
struct FSeatTransformChunk
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FTransform> Transforms;
};

UCLASS(meta = (PrioritizeCategories = "Parm"))
class STADIUM56_API AAGlobalSeatManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAGlobalSeatManager();

	// called by ASeatSpawner to register Transforms
	UFUNCTION(BlueprintCallable, Category = "Parm")
	void RegisterSeatChunk(AActor* Spawner, const TArray<FTransform>& RawTransforms);

	// remove from manager when destroyed
	void UnregisterSeatChunk(AActor* Spawner);

	// all called seat transforms
	TArray<FTransform> AllTransforms;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override; 
		
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	USceneComponent* DefaultSceneRoot;

	UPROPERTY(BlueprintReadWrite)
	UHierarchicalInstancedStaticMeshComponent* SeatGridHISM;

	UPROPERTY(EditDefaultsOnly, Category = "Parm|Seat")
	UStaticMesh* SeatMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parm|Seat")
	FRotator SeatRotationOffset;

	// debug cone
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parm|Debug")
	UStaticMeshComponent* DebugCone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parm|Debug")
	bool bUseDebugMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parm|Debug")
	FRotator ConeRotationOffset;

	// clean and rebuild
	UFUNCTION(BlueprintCallable, Category = "Parm")
	void RebuildHISMs();

private:
	// store all seat transforms
	TMap<TWeakObjectPtr<AActor>, TArray<FTransform>> ChunkData;

	// internal, set seat vs cone
	void UpdateHISMVisuals();

	// conbine and apply BP global transforms
	void CombineTransforms(TArray<FTransform>& OutTransforms);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
