// Fill out your copyright notice in the Description page of Project Settings.


#include "StandsSystem/AGlobalCrowdManager.h"
#include "StandsSystem/AGlobalSeatManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "StandsSystem/ACrowdVolume.h"
#include "EngineUtils.h"

// Sets default values
AAGlobalCrowdManager::AAGlobalCrowdManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	HISMsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HISMsRoot"));
	HISMsRoot->SetupAttachment(RootComponent);

	//bBakeCrowd = false;
	bHasInitialBaked = false;
}

void AAGlobalCrowdManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SeatManager && !bHasInitialBaked)
	{
		BakeCrowd();
		bHasInitialBaked = true;
	}
}

void AAGlobalCrowdManager::ClearCrowd()
{
	for (UHierarchicalInstancedStaticMeshComponent* HISM : CrowdHISMs)
	{
		if (HISM)
		{
			HISM->ClearInstances();
		}
	}
}

void AAGlobalCrowdManager::SetupHISMComponents()
{
	const int32 NumVariants = CrowdCharacterVariants.Num();
	const int32 NumMatsPerVariant = CrowdCharacterVariants.Num() > 0 ? CrowdCharacterVariants[0].VATMats.Num() : 0;
	const int32 TotalHISMsNeeded = NumVariants * NumMatsPerVariant;

	bool bIsHISMsInvalid = (CrowdHISMs.Num() != TotalHISMsNeeded);
	if (!bIsHISMsInvalid)
	{
		for (UHierarchicalInstancedStaticMeshComponent* HISM : CrowdHISMs)
		{
			if (!HISM)
			{
				bIsHISMsInvalid = true;
				break;
			}
		}
	}

	//initialize
	if (bIsHISMsInvalid)
	{
		// cleanup
		for (UHierarchicalInstancedStaticMeshComponent* HISM : CrowdHISMs)
		{
			if (HISM && HISM->IsValidLowLevel())
			{
				HISM->ClearInstances();
				HISM->DestroyComponent();
			}
		}
		CrowdHISMs.Empty(TotalHISMsNeeded);

		// create new
		for (int32 i = 0; i < TotalHISMsNeeded; ++i)
		{
			FName HismName = FName(*FString::Printf(TEXT("CrowdHISM_%d"), i));
			UHierarchicalInstancedStaticMeshComponent* NewHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, HismName);
			NewHISM->SetupAttachment(HISMsRoot);
			NewHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			NewHISM->RegisterComponent();

			// enable custom data!!!!!!
			NewHISM->NumCustomDataFloats = 1;

			CrowdHISMs.Add(NewHISM);
		}
	}

	// setup 
	for (int32 VariantIdx = 0; VariantIdx < NumVariants; ++VariantIdx)
	{
		const FCharacterVariant& Variant = CrowdCharacterVariants[VariantIdx];
		if (!Variant.Mesh) continue; // empty

		for (int32 MatIdx = 0; MatIdx < NumMatsPerVariant; ++MatIdx)
		{
			int32 HISM_Index = (VariantIdx * NumMatsPerVariant) + MatIdx;

			if (CrowdHISMs.IsValidIndex(HISM_Index) && CrowdHISMs[HISM_Index] && Variant.VATMats.IsValidIndex(MatIdx))
			{
				CrowdHISMs[HISM_Index]->SetStaticMesh(Variant.Mesh);
				CrowdHISMs[HISM_Index]->SetMaterial(0, Variant.VATMats[MatIdx]);
			}
		}
	}
}

TArray<FTransform> AAGlobalCrowdManager::GetFilteredSeatTransforms() const
{
	TArray<FTransform> FilteredSeats;

	if (!SeatManager)
	{
		return FilteredSeats;
	}

	// 1. get transforms from seat manager
	const TArray<FTransform> AllSeats = SeatManager->AllTransforms;

	if (AllSeats.Num() == 0) return FilteredSeats;

	// 2. search ACrowdVolume
	TArray<AACrowdVolume*> Volumes;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AACrowdVolume> It(World); It; ++It)
		{
			Volumes.Add(*It);
		}
	}

	if (Volumes.Num() == 0) return FilteredSeats;


	// 4. expensive: filter seats by volumes
	for (const FTransform& SeatTransform : AllSeats)
	{
		const FVector SeatLocation = SeatTransform.GetLocation();

		for (AACrowdVolume* Volume : Volumes)
		{
			if (Volume && Volume->GetQueryBox().IsInside(SeatLocation))
			{
				// density from volume
				FRandomStream Stream(Volume->RandomSeed + FMath::TruncToInt(SeatLocation.X + SeatLocation.Y));

				if (Stream.GetFraction() < Volume->CrowdDensity)
				{
					FilteredSeats.Add(SeatTransform);
				}

				// found a volume. stop check for this seat
				break;
			}
		}
	}

	return FilteredSeats;
}

void AAGlobalCrowdManager::PopulateHISMs(const TArray<FTransform>& FilteredSeats)
{
	const int32 NumMeshes = CrowdCharacterVariants.Num(); 
	// get zeroth's mat num
	const int32 NumMats = CrowdCharacterVariants.Num() > 0 ? CrowdCharacterVariants[0].VATMats.Num() : 0;
	const int32 TotalHISMs = CrowdHISMs.Num();

	// validate
	if (NumMats == 0 || TotalHISMs == 0) return;

	// transform of each hism
	TArray<TArray<FTransform>> HismTransforms;
	HismTransforms.SetNum(TotalHISMs);

	// custom data of each hism    -> for random time offset
	TArray<TArray<float>> HismCustomData;
	HismCustomData.SetNum(TotalHISMs);

	// deprecated, invert in bp onconstruction
	//const FTransform ManagerInverseWorldTransform = GetActorTransform().Inverse();


	for (const FTransform& WorldSeatTransform : FilteredSeats)
	{
		// select a combination of mesh and mat
		const int32 MeshIdx = FMath::RandRange(0, NumMeshes - 1);

		// weighted pick
		const FCharacterVariant& Variant = CrowdCharacterVariants[MeshIdx];
		const int32 MatIdx = PickMIByWeight();
		if (MatIdx == -1) continue; //no mat found

		const int32 HismIndex = (MeshIdx * NumMats) + MatIdx;

		const float RandomDataForThisInstance = FMath::FRand();

		HismTransforms[HismIndex].Add(OffsetTransform * WorldSeatTransform);
		HismCustomData[HismIndex].Add(RandomDataForThisInstance);
	}

	for (int32 i = 0; i < TotalHISMs; ++i)
	{
		if (CrowdHISMs[i] && HismTransforms[i].Num() > 0)
		{
			// add instances in batches
			CrowdHISMs[i]->AddInstances(HismTransforms[i], true);

			// add custom data one by one
			const TArray<float>& CustomDataForThisHISM = HismCustomData[i];
			const int32 NumInstancesInThisHISM = CustomDataForThisHISM.Num();
			for (int32 j = 0; j < NumInstancesInThisHISM; ++j)
			{
				// custom index = 0
				CrowdHISMs[i]->SetCustomDataValue(j, 0, CustomDataForThisHISM[j]);
			}
		}
	}
}

void AAGlobalCrowdManager::BakeCrowd()
{
	// 1. 
	ClearCrowd();

	// 2. 
	SetupHISMComponents();

	// 3. expensive search
	const TArray<FTransform> FilteredSeats = GetFilteredSeatTransforms();

	// 4. fillup hisms
	PopulateHISMs(FilteredSeats);

	UE_LOG(LogTemp, Log, TEXT("Crowd Baked %d instances"), FilteredSeats.Num());
}

int32 AAGlobalCrowdManager::PickMIByWeight() const
{
	// mi slots
	const int32 NumMats = (CrowdCharacterVariants.Num() > 0)
		? CrowdCharacterVariants[0].VATMats.Num()
		: 0;

	// 2. weight slots
	const int32 NumWeights = MaterialWeights.GetNumWeights();

	// 3. get min one
	const int32 NumOptions = FMath::Min(NumMats, NumWeights);

	if (NumOptions == 0)
	{
		return -1;
	}

	// 4. sum
	float sum = 0.0f;
	for (int32 i = 0; i < NumOptions; ++i)
		sum += MaterialWeights.GetWeightByIndex(i);

	if (sum <= 0.0f)
		return FMath::RandRange(0, NumOptions - 1); // then pure random

	// 5. pick by weight
	float Roll = FMath::FRand() * sum;

	// 6. find the domain
	float CurrentWeightSum = 0.0f;
	for (int32 i = 0; i < NumOptions; ++i)
	{
		CurrentWeightSum += MaterialWeights.GetWeightByIndex(i);
		if (Roll < CurrentWeightSum)
		{
			return i; // found
		}
	}

	return NumOptions - 1;
}

// Called when the game starts or when spawned
void AAGlobalCrowdManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAGlobalCrowdManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

