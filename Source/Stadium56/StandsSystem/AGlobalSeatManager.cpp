// Fill out your copyright notice in the Description page of Project Settings.


#include "StandsSystem/AGlobalSeatManager.h"
#include "StandsSystem/ASeatSpawnerBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AAGlobalSeatManager::AAGlobalSeatManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// HISM as root
	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;
	SeatGridHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("SeatGridHISM"));
	SeatGridHISM->SetupAttachment(RootComponent);
	SeatGridHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// debug cone
	DebugCone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugCone"));
	DebugCone->SetupAttachment(RootComponent);
	DebugCone->bHiddenInGame = true; // Debug
	DebugCone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugCone->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMeshAsset.Succeeded())
	{
		DebugCone->SetStaticMesh(ConeMeshAsset.Object);
	}

	// initialize rotations
	SeatRotationOffset = FRotator::ZeroRotator;
	ConeRotationOffset = FRotator(-90.0f, 0.0f, 0.0f);
}

// called by ASeatSpawner to register Transforms
void AAGlobalSeatManager::RegisterSeatChunk(AActor* Spawner, const TArray<FTransform>& RawTransforms)
{
	if (!Spawner) return;
	ChunkData.Add(Spawner, RawTransforms);
	RebuildHISMs();
}

// remove from manager when destroyed
void AAGlobalSeatManager::UnregisterSeatChunk(AActor* Spawner)
{
	if (!Spawner) return;
	if (ChunkData.Remove(Spawner) >0 )
	{
		RebuildHISMs();
	}
}

void AAGlobalSeatManager::UpdateHISMVisuals()
{
	UStaticMesh* TargetMesh = nullptr;

	if (bUseDebugMesh)
	{
		if (DebugCone)
		{
			TargetMesh = DebugCone->GetStaticMesh();
		}
	}
	else
	{
		TargetMesh = SeatMesh;
	}

	if (SeatGridHISM)
	{
		if (TargetMesh != SeatGridHISM->GetStaticMesh())
		{
			SeatGridHISM->SetStaticMesh(TargetMesh);
		}
	}
}

void AAGlobalSeatManager::RebuildHISMs()
{
	if (!SeatGridHISM) return;

	// 1. setup HISM
	UpdateHISMVisuals();

	// 2. clean all old instances
	SeatGridHISM->ClearInstances();

	// 3. validate
	const bool bHasValidMesh = (SeatGridHISM->GetStaticMesh() != nullptr);
	if (!bHasValidMesh)
	{
		SeatGridHISM->SetVisibility(false);
		return;
	}

	// 4. combine all Transforms
	AllTransforms.Empty();
	CombineTransforms(AllTransforms);

	// 5. add to HISM
	if (AllTransforms.Num() > 0)
	{
		SeatGridHISM->SetVisibility(true);
		SeatGridHISM->AddInstances(AllTransforms, false);
	}
	else
	{
		SeatGridHISM->SetVisibility(false);
	}
}

void AAGlobalSeatManager::CombineTransforms(TArray<FTransform>& OutTransforms)
{

	const FVector IndividualScale = bUseDebugMesh ? FVector(0.5f) : FVector(1.0f); 
	const FRotator IndividualRotation = bUseDebugMesh ? ConeRotationOffset : SeatRotationOffset;

	for (const TPair<TWeakObjectPtr<AActor>, TArray<FTransform>>& Pair : ChunkData)
	{
		if (AASeatSpawnerBase* Spawner = Cast<AASeatSpawnerBase>(Pair.Key.Get()))
		{
			const FRotator BaseRotation = Spawner->GetLocalForwardDirection().Rotation(); 

			FTransform SpawnerWorldTransform = Spawner->GetActorTransform();
			SpawnerWorldTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));

			
			for (const FTransform& RawTransform : Pair.Value)
			{
				const FTransform FinalLocalTransform(
					BaseRotation + IndividualRotation,
					RawTransform.GetLocation(),
					IndividualScale
				);
				OutTransforms.Add(FinalLocalTransform * SpawnerWorldTransform);
			}
		}
	}
}

// Called when the game starts or when spawned
void AAGlobalSeatManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAGlobalSeatManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

