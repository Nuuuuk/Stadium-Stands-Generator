// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "StandsSystem/ACrowdVolume.h"
#include "AGlobalCrowdManager.generated.h"

class AAGlobalSeatManager;

// 3sm * 6MI vat
USTRUCT(BlueprintType)
struct FCharacterVariant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset")
	UStaticMesh* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset")
	TArray<UMaterialInterface*> VATMats;

	FCharacterVariant() //default
	{
		Mesh = nullptr;
	}
};

UCLASS(meta = (PrioritizeCategories = "Parm"))
class STADIUM56_API AAGlobalCrowdManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAGlobalCrowdManager(); 
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Parm", meta = (CallInEditor = "true"))
	void BakeCrowd();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	USceneComponent* DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	USceneComponent* HISMsRoot;

	//hook the seat manager in bp
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parm")
	AAGlobalSeatManager* SeatManager;

	//// a fake button. will auto uncheck
	//UPROPERTY(EditAnywhere, Category = "Parm|Bake")
	//bool bBakeCrowd;

	// offset to each character
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parm|Assets")
	FTransform OffsetTransform;

	// sm * mi
	UPROPERTY(EditDefaultsOnly, Category = "Parm|Assets")
	TArray<FCharacterVariant> CrowdCharacterVariants;

	UPROPERTY(VisibleAnywhere, Category = "Parm")
	TArray<UHierarchicalInstancedStaticMeshComponent*> CrowdHISMs;

private:
	// bake when spawned first timne
	UPROPERTY()
	bool bHasInitialBaked;

	// clean HISMs
	void ClearCrowd();

	// expensive. get all seat transforms, and filter them by crowd volumes
	TArray<FTransform> GetFilteredSeatTransforms() const;

	void SetupHISMComponents();

	// randomly assign crowd to HISMs
	void PopulateHISMs(const TArray<FTransform>& FilteredSeats);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
