// Fill out your copyright notice in the Description page of Project Settings.


#include "StandsSystem/ASeatSpawnerBase.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
AASeatSpawnerBase::AASeatSpawnerBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SeatSpline = CreateDefaultSubobject<USplineComponent>(TEXT("SeatSpline"));
	RootComponent = SeatSpline;
	LocalForwardDirection = - FVector::ForwardVector; // (-1, 0, 0)
	
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
	
	bShowDebugCone = true;
	
	// initialize  offsets
	ColumnSpacing = 100.0f;
	RowSpacing = 150.0f;
	RowHeightOffset = 50.0f;

	// Debug cones ISM
	DebugSeatGridISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DebugSeatGridISM"));
	DebugSeatGridISM->SetupAttachment(RootComponent);
	DebugSeatGridISM->bHiddenInGame = true;
	DebugSeatGridISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugSeatGridISM->SetVisibility(false);

	if (ConeMeshAsset.Succeeded())
	{
		DebugSeatGridISM->SetStaticMesh(ConeMeshAsset.Object);
	}

	// default spline points
	if (WITH_EDITOR)
	{
		SeatSpline->ClearSplinePoints(false); // clear default pts

		SeatSpline->AddSplinePoint(FVector(0.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
		SeatSpline->AddSplinePoint(FVector(1000.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
		SeatSpline->AddSplinePoint(FVector(1000.f, 1000.f, 0.f), ESplineCoordinateSpace::Local, false);
		SeatSpline->AddSplinePoint(FVector(0.f, 1000.f, 0.f), ESplineCoordinateSpace::Local, false); 
		
		SeatSpline->SetSplinePointType(0, ESplinePointType::Linear, false);
		SeatSpline->SetSplinePointType(1, ESplinePointType::Linear, false);
		SeatSpline->SetSplinePointType(2, ESplinePointType::Linear, false);
		SeatSpline->SetSplinePointType(3, ESplinePointType::Linear, false);

		// closed 
		SeatSpline->SetClosedLoop(true, false);

		SeatSpline->UpdateSpline();
	}
}

void AASeatSpawnerBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	// lock point 0
	if (SeatSpline && SeatSpline->GetNumberOfSplinePoints() > 0)
	{
		const FVector Point0Location = SeatSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::Local);

		// if it's not at (0,0,0)
		if (!Point0Location.IsZero())
		{
			SeatSpline->SetLocationAtSplinePoint(0, FVector::ZeroVector, ESplineCoordinateSpace::Local, true);
		}
	}

	// show debug cone
	if (bShowDebugCone && SeatSpline && SeatSpline->GetNumberOfSplinePoints() > 0)
	{
		DebugCone->SetVisibility(true);

		// cone to pt0
		FVector Point0Location = SeatSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::Local);
		DebugCone->SetRelativeLocation(Point0Location);

		// to forward direction
		FRotator TargetDirectionRotator = LocalForwardDirection.Rotation();
		FRotator MeshOffsetRotator = FRotator(-90.0f, 00.0f, 0.0f);

		DebugCone->SetRelativeRotation(TargetDirectionRotator + MeshOffsetRotator);
	}
	else
	{
		DebugCone->SetVisibility(false);
	}


	// save the transforms
	TArray<FTransform> GeneratedTransforms;

	// debug cone ISM
	if (DebugSeatGridISM)
	{
		DebugSeatGridISM->ClearInstances();
	}
	if (bShowDebugCone && DebugSeatGridISM)
	{
		DebugSeatGridISM->SetVisibility(true);

		const FVector ForwardVector = -LocalForwardDirection;
		const FVector RightVector = FVector::CrossProduct(ForwardVector, FVector::UpVector);
		// copy the rotaion of cone0
		const FRotator ConeRotation = LocalForwardDirection.Rotation() + FRotator(-90.0f, 0.0f, 0.0f);
		const FVector StartPosition = FVector::ZeroVector;

		// Debug 4x4
		for (int32 Row = 0; Row < 4; ++Row)
		{
			const FVector RowOffset = ForwardVector * Row * RowSpacing;
			const FVector HeightOffset = FVector::UpVector * Row * RowHeightOffset;

			for (int32 Col = 0; Col < 4; ++Col)
			{
				const FVector ColumnOffset = RightVector * Col * ColumnSpacing;

				const FVector FinalPosition = StartPosition + RowOffset + HeightOffset + ColumnOffset;

				const FTransform InstanceTransform(ConeRotation, FinalPosition);
				
				GeneratedTransforms.Add(InstanceTransform);
			}
		}

		// TArray to fill ISM
		for (const FTransform& Transform : GeneratedTransforms)
		{
			DebugSeatGridISM->AddInstance(Transform);
		}
	}
	else if (DebugSeatGridISM)
	{
		DebugSeatGridISM->SetVisibility(false);
	}
}

// Called when the game starts or when spawned
void AASeatSpawnerBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AASeatSpawnerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

