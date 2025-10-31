// Fill out your copyright notice in the Description page of Project Settings.


#include "StandsSystem/ASeatSpawnerBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/InstancedStaticMeshComponent.h"

// if a 2D point is inside an any polygon
static bool IsPointInPolygon2D(const FVector2D& Point, const TArray<FVector2D>& PolygonVertices)
{
	bool bIsInside = false;
	const int32 NumVerts = PolygonVertices.Num();
	if (NumVerts < 3)
	{
		return false; // invalid polygon
	}

	for (int32 i = 0, j = NumVerts - 1; i < NumVerts; j = i++)
	{
		const FVector2D& Vi = PolygonVertices[i];
		const FVector2D& Vj = PolygonVertices[j];

		// odd-even rule
		// if radial line crosses line segment (Vi.Y, Vj.Y)
		if (((Vi.Y > Point.Y) != (Vj.Y > Point.Y)) && (Point.X < (Vj.X - Vi.X) * (Point.Y - Vi.Y) / (Vj.Y - Vi.Y) + Vi.X))
		{
			bIsInside = !bIsInside;
		}
	}
	return bIsInside;
}

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



	// clean all old instances
	if (DebugSeatGridISM)
	{
		DebugSeatGridISM->ClearInstances();
	}



	// 2D spline points
	TArray<FVector2D> SplinePoints2D;

	if (SeatSpline)
	{
		const int32 NumSplinePoints = SeatSpline->GetNumberOfSplinePoints(); //ordered
		for (int32 i = 0; i < NumSplinePoints; ++i)
		{
			const FVector Location3D = SeatSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
			SplinePoints2D.Add(FVector2D(Location3D.X, Location3D.Y));
		}
	}

	// save the transforms
	TArray<FTransform> GeneratedTransforms;

	if (bShowDebugCone && DebugSeatGridISM && SplinePoints2D.Num() > 2)
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

				const FVector2D SeatLocation2D(FinalPosition.X, FinalPosition.Y);
				// check if inside spline polygon
				if (IsPointInPolygon2D(SeatLocation2D, SplinePoints2D))
				{
					const FTransform InstanceTransform(ConeRotation, FinalPosition);
					GeneratedTransforms.Add(InstanceTransform);
				}
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

