// Fill out your copyright notice in the Description page of Project Settings.


#include "StandsSystem/ASeatSpawnerBase.h"
#include "StandsSystem/AGlobalSeatManager.h"

// Deprecated
/**
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
**/

// points on a 2D row that are inside a polygon
static void FindVerticalScanlineIntersections(float ScanlineX,
	const TArray<FVector2D>& PolygonVertices,
	TArray<float>& OutYIntersections)
{
	const int32 NumVerts = PolygonVertices.Num();
	if (NumVerts < 3)
	{
		return;
	}

	for (int32 i = 0, j = NumVerts - 1; i < NumVerts; j = i++)
	{
		const FVector2D& Vi = PolygonVertices[i];
		const FVector2D& Vj = PolygonVertices[j];

		if (((Vi.X > ScanlineX) != (Vj.X > ScanlineX)))
		{
			const float IntersectY = (Vj.Y - Vi.Y) * (ScanlineX - Vi.X) / (Vj.X - Vi.X) + Vi.Y;
			OutYIntersections.Add(IntersectY);
		}
	}
}

// Sets default values
AASeatSpawnerBase::AASeatSpawnerBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SeatSpline = CreateDefaultSubobject<USplineComponent>(TEXT("SeatSpline"));
	RootComponent = SeatSpline;
	LocalForwardDirection = - FVector::ForwardVector; // (-1, 0, 0)
	
	// initialize  offsets
	ColumnSpacing = 100.0f;
	RowSpacing = 150.0f;


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

void AASeatSpawnerBase::UpdateAndValidateSpline()
{
	// lock point 0
	if (SeatSpline && SeatSpline->GetNumberOfSplinePoints() > 0)
	{
		bool bSplineWasModified = false;
		const int32 NumPoints = SeatSpline->GetNumberOfSplinePoints();
		for (int32 i = 0; i < NumPoints; ++i)
		{
			FVector PointLocation = SeatSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
			if (i == 0)
			{
				if (!PointLocation.IsZero()) // if pt 0 not at origin
				{
					SeatSpline->SetLocationAtSplinePoint(0, FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
					bSplineWasModified = true;
				}
			}
			else
			{
				if (PointLocation.Z < 0.0f) // if pt z < 0
				{
					SeatSpline->SetLocationAtSplinePoint(i, FVector(PointLocation.X, PointLocation.Y, 0.0f), ESplineCoordinateSpace::Local, false);
					bSplineWasModified = true;
				}
			}
		}

		if (bSplineWasModified)
		{
			SeatSpline->UpdateSpline();
		}
	}
}


TArray<FTransform> AASeatSpawnerBase::GenerateTransforms()
{
	// save the transforms
	TArray<FTransform> GeneratedTransforms;

	// 2D spline points
	TArray<FVector2D> SplinePoints2D;
	// spline bounds 3d
	FBox SplineBounds(ForceInit);

	if (!SeatSpline || SeatSpline->GetNumberOfSplinePoints() <= 2)
	{
		return GeneratedTransforms; // return empty
	}

	// spline is root so actor scale = spline scale
	const FVector ActorScale = GetActorScale3D();

	const int32 NumSplinePoints = SeatSpline->GetNumberOfSplinePoints(); //ordered
	for (int32 i = 0; i < NumSplinePoints; ++i)
	{
		const FVector Location3D = SeatSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
		const FVector Location3D_Scaled = Location3D * ActorScale;
		SplinePoints2D.Add(FVector2D(Location3D_Scaled.X, Location3D_Scaled.Y));
		// add pt to bounds
		SplineBounds += Location3D_Scaled;
	}


	const FRotator BaseRotation = LocalForwardDirection.Rotation();

	// save the intersections of a row and the spline
	TArray<float> YIntersections;

	//// AABB to get row index range
	const int32 MinRow = FMath::FloorToInt(SplineBounds.Min.X / RowSpacing);
	const int32 MaxRow = FMath::CeilToInt(SplineBounds.Max.X / RowSpacing);

	// Calculate Z offset
	const float TotalHeight = SplineBounds.Max.Z;
	const float SplineXSize = SplineBounds.GetSize().X;

	for (int32 Row = MinRow; Row <= MaxRow; ++Row)
	{
		YIntersections.Reset(); //clear previous row

		const float ScanlineX = Row * RowSpacing;
		float Z_Height = 0.0f;
		if (SplineXSize > KINDA_SMALL_NUMBER) 
		{
			// cal the persentage of ScanlineX in aabb
			const float Alpha = (ScanlineX - SplineBounds.Min.X) / SplineXSize;
			Z_Height = FMath::Lerp(0.0f, TotalHeight, Alpha);
		}
		FindVerticalScanlineIntersections(ScanlineX, SplinePoints2D, YIntersections);

		if (YIntersections.Num() < 2)
		{
			continue; // no intersections
		}

		YIntersections.Sort();

		// fill inside. odd-even rule
		for (int32 i = 0; i < YIntersections.Num(); i += 2)
		{
			const float Y_Enter = YIntersections[i];
			const float Y_Exit = YIntersections[i + 1];

			// AABB to get column index range
			const int32 MinCol = FMath::CeilToInt(Y_Enter / ColumnSpacing);
			const int32 MaxCol = FMath::FloorToInt(Y_Exit / ColumnSpacing);
			//for (int32 Col = 0; Col < 5; ++Col)
			for (int32 Col = MinCol; Col <= MaxCol; ++Col)
			{
				const float SeatY = Col * ColumnSpacing;

				const FVector FinalPosition(ScanlineX, SeatY, Z_Height);
				const FTransform InstanceTransform(BaseRotation, FinalPosition);
				GeneratedTransforms.Add(InstanceTransform);
			}
		}
	}

	return GeneratedTransforms;
}

void AASeatSpawnerBase::Destroyed()
{
	if (SeatManager)
	{
		SeatManager->UnregisterSeatChunk(this);
	}
	Super::Destroyed();
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

