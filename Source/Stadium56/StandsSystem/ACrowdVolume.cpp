// Fill out your copyright notice in the Description page of Project Settings.


#include "StandsSystem/ACrowdVolume.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "StandsSystem/AGlobalCrowdManager.h"
#include "EngineUtils.h"

// Sets default values
AACrowdVolume::AACrowdVolume()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;


	QueryBox = CreateDefaultSubobject<UBoxComponent>(TEXT("QueryBox"));
	RootComponent = QueryBox;
	QueryBox->SetBoxExtent(FVector(500.f, 400.f, 300.f)); // 5m x 4m x 3m

	CrowdDensity = 0.8f; // defaykt
	RandomSeed = -487486592;
}

void AACrowdVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// random seed on first spawn
	if (RandomSeed == -487486592)
	{
		RandomSeed = FMath::RandRange(0, 999999);
	}
}

FBox AACrowdVolume::GetQueryBox() const
{
	if (QueryBox)
	{
		// world bound
		return QueryBox->CalcBounds(GetActorTransform()).GetBox();
	}
	return FBox(ForceInit);
}

void AACrowdVolume::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	// bFinished - > move eended
	if (bFinished && CrowdManager)
	{
		CrowdManager->BakeCrowd();
	}
}

void AACrowdVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!CrowdManager) return;
	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive) return;
	CrowdManager->BakeCrowd();
}

// Called when the game starts or when spawned
void AACrowdVolume::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AACrowdVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

