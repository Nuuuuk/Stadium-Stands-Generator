// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AGlobalSeatManager.generated.h"

UCLASS(meta = (PrioritizeCategories = "_Parm_"))
class STADIUM56_API AAGlobalSeatManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAGlobalSeatManager();

	// called by ASeatSpawner to register Transforms
	void RegisterSeatChunk(AActor* Spawner, const TArray<FTransform>& RawTransforms);

	// remove from manager when destroyed
	void UnregisterSeatChunk(AActor* Spawner);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override; 
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	USceneComponent* DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "_Parm_|Seat")
	UHierarchicalInstancedStaticMeshComponent* SeatGridHISM;

	UPROPERTY(EditDefaultsOnly, Category = "_Parm_|Seat")
	UStaticMesh* SeatMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "_Parm_|Seat")
	FRotator SeatRotationOffset;

	// debug cone
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "_Parm_|Debug")
	UStaticMeshComponent* DebugCone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "_Parm_|Debug")
	bool bUseDebugMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "_Parm_|Debug")
	FRotator ConeRotationOffset;

private:
	// store all seat transforms
	TMap<TWeakObjectPtr<AActor>, TArray<FTransform>> ChunkData;

	// clean and rebuild
	void RebuildHISMs();

	// internal, set seat vs cone
	void UpdateHISMVisuals();

	// conbine and apply BP global transforms
	void CombineTransforms(TArray<FTransform>& AllTransforms);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
