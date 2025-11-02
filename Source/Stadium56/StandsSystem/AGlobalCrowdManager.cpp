// Fill out your copyright notice in the Description page of Project Settings.


#include "StandsSystem/AGlobalCrowdManager.h"
#include "StandsSystem/AGlobalSeatManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"

// Sets default values
AAGlobalCrowdManager::AAGlobalCrowdManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	bBakeCrowd = false;
}

void AAGlobalCrowdManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBakeCrowd)
	{
		BakeCrowd();
		bBakeCrowd = false; //invert the button
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
	const int32 NumMatsPerVariant = 6;
	const int32 TotalHISMsNeeded = NumVariants * NumMatsPerVariant;

	//initialize
	if (CrowdHISMs.Num() != TotalHISMsNeeded)
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
			NewHISM->SetupAttachment(RootComponent);
			NewHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			NewHISM->RegisterComponent();
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
	const int32 NumVariants = CrowdCharacterVariants.Num();
	if (NumVariants == 0) return;

	const int32 NumMatsPerVariant = 6;
	const FTransform ManagerInverseWorldTransform = GetActorTransform().Inverse();
	FRandomStream Stream;

	for (const FTransform& WorldTransform : FilteredSeats)
	{
		const int32 VariantIndex = Stream.RandRange(0, NumVariants - 1);
		const int32 MatIndex = Stream.RandRange(0, NumMatsPerVariant - 1);

		const int32 HISM_Index = (VariantIndex * NumMatsPerVariant) + MatIndex;

		if (CrowdHISMs.IsValidIndex(HISM_Index) && CrowdHISMs[HISM_Index])
		{
			CrowdHISMs[HISM_Index]->AddInstance(WorldTransform * ManagerInverseWorldTransform);
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

