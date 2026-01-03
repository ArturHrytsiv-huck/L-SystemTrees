// TreeMath.cpp
// Mathematical utilities for tree geometry and turtle interpretation
// Part of LSystemTrees Plugin - Phase 3

#include "Core/Utilities/TreeMath.h"

// ============================================================================
// Vector Rotation
// ============================================================================

FVector UTreeMath::RotateVectorAroundAxis(const FVector& Vector, const FVector& Axis, float AngleDegrees)
{
	// Use Rodrigues' rotation formula
	// v_rot = v*cos(θ) + (k×v)*sin(θ) + k*(k·v)*(1-cos(θ))
	// where k is the unit axis vector and θ is the angle

	if (Axis.IsNearlyZero())
	{
		return Vector;
	}

	const FVector K = Axis.GetSafeNormal();
	const float AngleRad = FMath::DegreesToRadians(AngleDegrees);
	const float CosAngle = FMath::Cos(AngleRad);
	const float SinAngle = FMath::Sin(AngleRad);

	// Rodrigues' formula components
	const FVector Term1 = Vector * CosAngle;
	const FVector Term2 = FVector::CrossProduct(K, Vector) * SinAngle;
	const FVector Term3 = K * FVector::DotProduct(K, Vector) * (1.0f - CosAngle);

	return Term1 + Term2 + Term3;
}

FVector UTreeMath::RotateAroundX(const FVector& Vector, float AngleDegrees)
{
	const float AngleRad = FMath::DegreesToRadians(AngleDegrees);
	const float CosAngle = FMath::Cos(AngleRad);
	const float SinAngle = FMath::Sin(AngleRad);

	return FVector(
		Vector.X,
		Vector.Y * CosAngle - Vector.Z * SinAngle,
		Vector.Y * SinAngle + Vector.Z * CosAngle
	);
}

FVector UTreeMath::RotateAroundY(const FVector& Vector, float AngleDegrees)
{
	const float AngleRad = FMath::DegreesToRadians(AngleDegrees);
	const float CosAngle = FMath::Cos(AngleRad);
	const float SinAngle = FMath::Sin(AngleRad);

	return FVector(
		Vector.X * CosAngle + Vector.Z * SinAngle,
		Vector.Y,
		-Vector.X * SinAngle + Vector.Z * CosAngle
	);
}

FVector UTreeMath::RotateAroundZ(const FVector& Vector, float AngleDegrees)
{
	const float AngleRad = FMath::DegreesToRadians(AngleDegrees);
	const float CosAngle = FMath::Cos(AngleRad);
	const float SinAngle = FMath::Sin(AngleRad);

	return FVector(
		Vector.X * CosAngle - Vector.Y * SinAngle,
		Vector.X * SinAngle + Vector.Y * CosAngle,
		Vector.Z
	);
}

// ============================================================================
// Basis Vector Operations
// ============================================================================

void UTreeMath::ReorthogonalizeBasis(FVector& Forward, FVector& Left, FVector& Up)
{
	// Gram-Schmidt orthogonalization with Forward as primary
	Forward = Forward.GetSafeNormal();

	// Make Left perpendicular to Forward
	Left = Left - Forward * FVector::DotProduct(Left, Forward);
	Left = Left.GetSafeNormal();

	// Handle degenerate case where Left became zero
	if (Left.IsNearlyZero())
	{
		// Pick a perpendicular vector
		if (FMath::Abs(Forward.Z) < 0.9f)
		{
			Left = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
		}
		else
		{
			Left = FVector::CrossProduct(FVector::ForwardVector, Forward).GetSafeNormal();
		}
	}

	// Up is cross product of Forward and Left
	Up = FVector::CrossProduct(Forward, Left).GetSafeNormal();
}

void UTreeMath::GetPerpendicularVectors(const FVector& Direction, FVector& OutRight, FVector& OutUp)
{
	const FVector NormalizedDir = Direction.GetSafeNormal();

	// Choose a reference vector that's not parallel to Direction
	FVector Reference;
	if (FMath::Abs(NormalizedDir.Z) < 0.9f)
	{
		Reference = FVector::UpVector;
	}
	else
	{
		Reference = FVector::ForwardVector;
	}

	// Calculate perpendicular vectors
	OutRight = FVector::CrossProduct(Reference, NormalizedDir).GetSafeNormal();
	OutUp = FVector::CrossProduct(NormalizedDir, OutRight).GetSafeNormal();
}

FQuat UTreeMath::MakeQuatFromBasis(const FVector& Forward, const FVector& Up)
{
	const FVector NormForward = Forward.GetSafeNormal();
	const FVector NormUp = Up.GetSafeNormal();

	// Create rotation matrix from basis
	const FVector Right = FVector::CrossProduct(NormUp, NormForward).GetSafeNormal();
	const FVector OrthUp = FVector::CrossProduct(NormForward, Right);

	FMatrix RotMatrix = FMatrix::Identity;
	RotMatrix.SetAxis(0, NormForward);
	RotMatrix.SetAxis(1, Right);
	RotMatrix.SetAxis(2, OrthUp);

	return FQuat(RotMatrix);
}

// ============================================================================
// Tree-Specific Calculations
// ============================================================================

float UTreeMath::CalculateChildWidth(float ParentWidth, int32 ChildCount, float Exponent)
{
	// Leonardo's rule: parent^n = sum(child_i^n)
	// For equal children: parent^n = N * child^n
	// Therefore: child = parent / N^(1/n)

	if (ChildCount <= 0 || ParentWidth <= 0.0f || Exponent <= 0.0f)
	{
		return ParentWidth;
	}

	const float Divisor = FMath::Pow(static_cast<float>(ChildCount), 1.0f / Exponent);
	return ParentWidth / Divisor;
}

float UTreeMath::CalculateWidthAtDepth(float InitialWidth, int32 Depth, float FalloffRate, float MinWidth)
{
	if (Depth <= 0)
	{
		return InitialWidth;
	}

	// Exponential falloff: width = initial * falloff^depth
	const float CalculatedWidth = InitialWidth * FMath::Pow(FalloffRate, static_cast<float>(Depth));
	return FMath::Max(CalculatedWidth, MinWidth);
}

FVector UTreeMath::ApplyTropism(const FVector& CurrentDirection, const FVector& TropismVector, float Strength)
{
	if (Strength <= 0.0f || TropismVector.IsNearlyZero())
	{
		return CurrentDirection.GetSafeNormal();
	}

	// Clamp strength to valid range
	const float ClampedStrength = FMath::Clamp(Strength, 0.0f, 1.0f);

	// Blend current direction toward tropism direction
	const FVector NormTropism = TropismVector.GetSafeNormal();
	const FVector BlendedDirection = FMath::Lerp(CurrentDirection, NormTropism, ClampedStrength);

	return BlendedDirection.GetSafeNormal();
}

// ============================================================================
// Geometry Helpers
// ============================================================================

FVector UTreeMath::GetPointOnCircle(const FVector& Center, const FVector& Normal, float Radius, float AngleDegrees)
{
	// Get perpendicular vectors to the normal
	FVector Right, Up;
	GetPerpendicularVectors(Normal, Right, Up);

	// Calculate point on circle
	const float AngleRad = FMath::DegreesToRadians(AngleDegrees);
	const FVector Offset = (Right * FMath::Cos(AngleRad) + Up * FMath::Sin(AngleRad)) * Radius;

	return Center + Offset;
}

TArray<FVector> UTreeMath::GenerateRingPoints(const FVector& Center, const FVector& Normal, float Radius, int32 NumPoints)
{
	TArray<FVector> Points;

	if (NumPoints <= 0 || Radius <= 0.0f)
	{
		return Points;
	}

	Points.Reserve(NumPoints);

	// Get perpendicular vectors
	FVector Right, Up;
	GetPerpendicularVectors(Normal, Right, Up);

	// Generate points
	const float AngleStep = 360.0f / static_cast<float>(NumPoints);
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const float AngleDegrees = AngleStep * static_cast<float>(i);
		const float AngleRad = FMath::DegreesToRadians(AngleDegrees);
		const FVector Offset = (Right * FMath::Cos(AngleRad) + Up * FMath::Sin(AngleRad)) * Radius;
		Points.Add(Center + Offset);
	}

	return Points;
}

FVector UTreeMath::CalculateTriangleNormal(const FVector& V0, const FVector& V1, const FVector& V2)
{
	const FVector Edge1 = V1 - V0;
	const FVector Edge2 = V2 - V0;
	return FVector::CrossProduct(Edge1, Edge2).GetSafeNormal();
}

float UTreeMath::LerpRadius(float StartRadius, float EndRadius, float T)
{
	return FMath::Lerp(StartRadius, EndRadius, FMath::Clamp(T, 0.0f, 1.0f));
}

// ============================================================================
// Random Utilities
// ============================================================================

float UTreeMath::RandomAngle(float MaxAngle, FRandomStream& RandomStream)
{
	return RandomStream.FRandRange(-MaxAngle, MaxAngle);
}

FVector UTreeMath::RandomDirectionInCone(const FVector& ConeAxis, float ConeAngle, FRandomStream& RandomStream)
{
	if (ConeAngle <= 0.0f)
	{
		return ConeAxis.GetSafeNormal();
	}

	// Generate random angle within cone
	const float HalfAngleRad = FMath::DegreesToRadians(ConeAngle);

	// Random angle around the axis
	const float Phi = RandomStream.FRandRange(0.0f, 2.0f * PI);

	// Random angle from axis (cosine-weighted for uniform distribution)
	const float CosTheta = RandomStream.FRandRange(FMath::Cos(HalfAngleRad), 1.0f);
	const float SinTheta = FMath::Sqrt(1.0f - CosTheta * CosTheta);

	// Build direction in local space
	const FVector LocalDir(
		SinTheta * FMath::Cos(Phi),
		SinTheta * FMath::Sin(Phi),
		CosTheta
	);

	// Transform to world space aligned with cone axis
	FVector Right, Up;
	GetPerpendicularVectors(ConeAxis, Right, Up);

	const FVector NormAxis = ConeAxis.GetSafeNormal();
	return (Right * LocalDir.X + Up * LocalDir.Y + NormAxis * LocalDir.Z).GetSafeNormal();
}
