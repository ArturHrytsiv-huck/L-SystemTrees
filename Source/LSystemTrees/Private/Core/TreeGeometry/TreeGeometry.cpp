// TreeGeometry.cpp
// Generates mesh geometry from branch segments and leaves
// Part of LSystemTrees Plugin - Phase 3

#include "Core/TreeGeometry/TreeGeometry.h"
#include "Core/Utilities/TreeMath.h"

DEFINE_LOG_CATEGORY(LogTreeGeometry);

// ============================================================================
// Constructor
// ============================================================================

UTreeGeometry::UTreeGeometry()
	: BarkUVTiling(1.0f)
	, DefaultLeafSize(10.0f, 15.0f)
	, CurrentVCoordinate(0.0f)
{
	RandomStream.Initialize(FMath::Rand());
}

// ============================================================================
// LOD Generation
// ============================================================================

TArray<FTreeMeshData> UTreeGeometry::GenerateMeshLODs(const TArray<FBranchSegment>& Segments,
                                                       const TArray<FLeafData>& Leaves,
                                                       const TArray<FTreeLODLevel>& LODLevels)
{
	TArray<FTreeMeshData> Results;

	if (LODLevels.Num() == 0)
	{
		UE_LOG(LogTreeGeometry, Warning, TEXT("No LOD levels specified, using default"));
		Results.Add(GenerateMesh(Segments, Leaves, 8, true));
		return Results;
	}

	Results.Reserve(LODLevels.Num());

	for (int32 i = 0; i < LODLevels.Num(); ++i)
	{
		const FTreeLODLevel& LOD = LODLevels[i];

		UE_LOG(LogTreeGeometry, Log, TEXT("Generating LOD %d: %d radial segments, leaves=%s"),
		       i, LOD.RadialSegments, LOD.bIncludeLeaves ? TEXT("true") : TEXT("false"));

		FTreeMeshData MeshData = GenerateMesh(Segments, Leaves, LOD.RadialSegments, LOD.bIncludeLeaves);
		Results.Add(MeshData);

		UE_LOG(LogTreeGeometry, Log, TEXT("LOD %d: %d vertices, %d triangles"),
		       i, MeshData.GetVertexCount(), MeshData.GetTriangleCount());
	}

	return Results;
}

FTreeMeshData UTreeGeometry::GenerateMesh(const TArray<FBranchSegment>& Segments,
                                           const TArray<FLeafData>& Leaves,
                                           int32 RadialSegments,
                                           bool bIncludeLeaves)
{
	ResetMeshData();

	// Clamp radial segments
	RadialSegments = FMath::Clamp(RadialSegments, 3, 32);
	CurrentRadialSegments = RadialSegments;

	// Clear segment tracking
	SegmentEndRingIndices.Empty();

	// Estimate capacity
	const int32 EstimatedBranchVertices = Segments.Num() * RadialSegments * 2;
	const int32 EstimatedLeafVertices = bIncludeLeaves ? Leaves.Num() * 4 : 0;
	const int32 TotalEstimatedVertices = EstimatedBranchVertices + EstimatedLeafVertices;

	CurrentMeshData.Vertices.Reserve(TotalEstimatedVertices);
	CurrentMeshData.Normals.Reserve(TotalEstimatedVertices);
	CurrentMeshData.UVs.Reserve(TotalEstimatedVertices);
	CurrentMeshData.Triangles.Reserve(Segments.Num() * RadialSegments * 6 + Leaves.Num() * 6);

	// Generate branch geometry with connectivity
	CurrentVCoordinate = 0.0f;
	for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
	{
		const FBranchSegment& Segment = Segments[SegmentIndex];
		GenerateBranchCylinderConnected(Segment, SegmentIndex, RadialSegments);
	}

	// Record branch counts
	CurrentMeshData.BranchVertexCount = CurrentMeshData.Vertices.Num();
	CurrentMeshData.BranchTriangleCount = CurrentMeshData.Triangles.Num() / 3;

	// Generate leaf geometry
	if (bIncludeLeaves)
	{
		for (const FLeafData& Leaf : Leaves)
		{
			GenerateLeafQuad(Leaf);
		}
	}

	// Calculate tangents for normal mapping
	CalculateTangents();

	UE_LOG(LogTreeGeometry, Verbose, TEXT("Generated mesh: %d branch verts, %d leaf verts, %d total triangles"),
	       CurrentMeshData.BranchVertexCount,
	       CurrentMeshData.Vertices.Num() - CurrentMeshData.BranchVertexCount,
	       CurrentMeshData.GetTriangleCount());

	return CurrentMeshData;
}

// ============================================================================
// Branch Geometry
// ============================================================================

void UTreeGeometry::GenerateBranchCylinder(const FBranchSegment& Segment, int32 RadialSegments)
{
	// Legacy function - calls connected version with no parent
	const float SegmentLength = Segment.GetLength();
	if (SegmentLength < KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Calculate UV V coordinates
	const float StartV = CurrentVCoordinate;
	const float EndV = StartV + (SegmentLength * BarkUVTiling / 100.0f);
	CurrentVCoordinate = EndV;

	// Generate rings at start and end of segment
	const int32 StartRingIndex = GenerateRing(Segment.StartPosition, Segment.Direction,
	                                           Segment.StartRadius, RadialSegments, StartV);
	const int32 EndRingIndex = GenerateRing(Segment.EndPosition, Segment.Direction,
	                                         Segment.EndRadius, RadialSegments, EndV);

	// Connect the rings with triangles
	ConnectRings(StartRingIndex, EndRingIndex, RadialSegments);
}

void UTreeGeometry::GenerateBranchCylinderConnected(const FBranchSegment& Segment, int32 SegmentIndex, int32 RadialSegments)
{
	const float SegmentLength = Segment.GetLength();
	if (SegmentLength < KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Calculate UV V coordinates
	const float StartV = CurrentVCoordinate;
	const float EndV = StartV + (SegmentLength * BarkUVTiling / 100.0f);
	CurrentVCoordinate = EndV;

	int32 StartRingIndex;

	// Check if this segment connects to a parent
	if (Segment.ParentSegmentIndex >= 0)
	{
		// Look up parent's end ring
		const int32* ParentEndRing = SegmentEndRingIndices.Find(Segment.ParentSegmentIndex);
		if (ParentEndRing != nullptr)
		{
			// Reuse parent's end ring as our start ring
			StartRingIndex = *ParentEndRing;
		}
		else
		{
			// Parent ring not found (shouldn't happen), generate new ring
			StartRingIndex = GenerateRing(Segment.StartPosition, Segment.Direction,
			                               Segment.StartRadius, RadialSegments, StartV);
		}
	}
	else
	{
		// No parent - generate a new start ring
		StartRingIndex = GenerateRing(Segment.StartPosition, Segment.Direction,
		                               Segment.StartRadius, RadialSegments, StartV);
	}

	// Always generate end ring
	const int32 EndRingIndex = GenerateRing(Segment.EndPosition, Segment.Direction,
	                                         Segment.EndRadius, RadialSegments, EndV);

	// Store this segment's end ring for children to use
	SegmentEndRingIndices.Add(SegmentIndex, EndRingIndex);

	// Connect the rings with triangles
	ConnectRings(StartRingIndex, EndRingIndex, RadialSegments);
}

int32 UTreeGeometry::GenerateRing(const FVector& Center, const FVector& Direction, float Radius,
                                   int32 NumSegments, float V)
{
	const int32 StartIndex = CurrentMeshData.Vertices.Num();

	// Get perpendicular vectors for the ring plane
	FVector Right, Up;
	GetPerpendicularVectors(Direction, Right, Up);

	// Generate vertices around the ring
	const float AngleStep = 360.0f / static_cast<float>(NumSegments);

	for (int32 i = 0; i < NumSegments; ++i)
	{
		const float Angle = AngleStep * static_cast<float>(i);
		const float AngleRad = FMath::DegreesToRadians(Angle);

		// Position on ring
		const FVector Offset = (Right * FMath::Cos(AngleRad) + Up * FMath::Sin(AngleRad)) * Radius;
		const FVector Position = Center + Offset;

		// Normal points outward from center
		const FVector Normal = Offset.GetSafeNormal();

		// UV coordinates: U goes around the ring, V goes along the branch
		const float U = static_cast<float>(i) / static_cast<float>(NumSegments);

		CurrentMeshData.Vertices.Add(Position);
		CurrentMeshData.Normals.Add(Normal);
		CurrentMeshData.UVs.Add(FVector2D(U, V));
		CurrentMeshData.VertexColors.Add(FLinearColor::White);
	}

	return StartIndex;
}

void UTreeGeometry::ConnectRings(int32 StartRingIndex, int32 EndRingIndex, int32 NumSegments)
{
	// Connect the two rings with a triangle strip
	for (int32 i = 0; i < NumSegments; ++i)
	{
		const int32 NextI = (i + 1) % NumSegments;

		// Indices for the quad
		const int32 A = StartRingIndex + i;
		const int32 B = StartRingIndex + NextI;
		const int32 C = EndRingIndex + i;
		const int32 D = EndRingIndex + NextI;

		// Triangle 1: A-C-B
		CurrentMeshData.Triangles.Add(A);
		CurrentMeshData.Triangles.Add(C);
		CurrentMeshData.Triangles.Add(B);

		// Triangle 2: B-C-D
		CurrentMeshData.Triangles.Add(B);
		CurrentMeshData.Triangles.Add(C);
		CurrentMeshData.Triangles.Add(D);
	}
}

// ============================================================================
// Leaf Geometry
// ============================================================================

void UTreeGeometry::GenerateLeafQuad(const FLeafData& Leaf)
{
	const int32 StartIndex = CurrentMeshData.Vertices.Num();

	// Get leaf orientation vectors
	FVector LeafRight, LeafUp;

	// Use the leaf's up direction, but ensure it's perpendicular to normal
	LeafUp = Leaf.UpDirection - Leaf.Normal * FVector::DotProduct(Leaf.UpDirection, Leaf.Normal);
	if (LeafUp.IsNearlyZero())
	{
		GetPerpendicularVectors(Leaf.Normal, LeafRight, LeafUp);
	}
	else
	{
		LeafUp.Normalize();
		LeafRight = FVector::CrossProduct(Leaf.Normal, LeafUp).GetSafeNormal();
	}

	// Apply random rotation around normal
	if (!FMath::IsNearlyZero(Leaf.Rotation))
	{
		const float RotRad = FMath::DegreesToRadians(Leaf.Rotation);
		const float Cos = FMath::Cos(RotRad);
		const float Sin = FMath::Sin(RotRad);

		const FVector NewRight = LeafRight * Cos + LeafUp * Sin;
		const FVector NewUp = -LeafRight * Sin + LeafUp * Cos;

		LeafRight = NewRight;
		LeafUp = NewUp;
	}

	// Get leaf size
	const FVector2D Size = Leaf.Size.IsNearlyZero() ? DefaultLeafSize : Leaf.Size;
	const float HalfWidth = Size.X * 0.5f;
	const float HalfHeight = Size.Y * 0.5f;

	// Generate 4 corners of the quad
	// Quad is centered at leaf position
	const FVector Corners[4] = {
		Leaf.Position - LeafRight * HalfWidth - LeafUp * HalfHeight, // Bottom-left
		Leaf.Position + LeafRight * HalfWidth - LeafUp * HalfHeight, // Bottom-right
		Leaf.Position + LeafRight * HalfWidth + LeafUp * HalfHeight, // Top-right
		Leaf.Position - LeafRight * HalfWidth + LeafUp * HalfHeight  // Top-left
	};

	// UV coordinates for leaf texture
	const FVector2D UVs[4] = {
		FVector2D(0.0f, 1.0f), // Bottom-left
		FVector2D(1.0f, 1.0f), // Bottom-right
		FVector2D(1.0f, 0.0f), // Top-right
		FVector2D(0.0f, 0.0f)  // Top-left
	};

	// Leaf color (could vary based on depth)
	const FLinearColor LeafColor = FLinearColor(0.2f, 0.6f, 0.2f, 1.0f);

	// Add vertices
	for (int32 i = 0; i < 4; ++i)
	{
		CurrentMeshData.Vertices.Add(Corners[i]);
		CurrentMeshData.Normals.Add(Leaf.Normal);
		CurrentMeshData.UVs.Add(UVs[i]);
		CurrentMeshData.VertexColors.Add(LeafColor);
	}

	// Add triangles (two triangles for the quad)
	// Triangle 1: 0-1-2
	CurrentMeshData.Triangles.Add(StartIndex + 0);
	CurrentMeshData.Triangles.Add(StartIndex + 1);
	CurrentMeshData.Triangles.Add(StartIndex + 2);

	// Triangle 2: 0-2-3
	CurrentMeshData.Triangles.Add(StartIndex + 0);
	CurrentMeshData.Triangles.Add(StartIndex + 2);
	CurrentMeshData.Triangles.Add(StartIndex + 3);

	// Add back faces for double-sided leaves
	// Triangle 3: 2-1-0
	CurrentMeshData.Triangles.Add(StartIndex + 2);
	CurrentMeshData.Triangles.Add(StartIndex + 1);
	CurrentMeshData.Triangles.Add(StartIndex + 0);

	// Triangle 4: 3-2-0
	CurrentMeshData.Triangles.Add(StartIndex + 3);
	CurrentMeshData.Triangles.Add(StartIndex + 2);
	CurrentMeshData.Triangles.Add(StartIndex + 0);
}

// ============================================================================
// Utility Methods
// ============================================================================

void UTreeGeometry::ResetMeshData()
{
	CurrentMeshData.Reset();
	CurrentVCoordinate = 0.0f;
}

void UTreeGeometry::GetPerpendicularVectors(const FVector& Direction, FVector& OutRight, FVector& OutUp)
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

void UTreeGeometry::CalculateSmoothNormals()
{
	// Normals are calculated per-vertex during generation
	// This method could be used for post-processing if needed
}

void UTreeGeometry::CalculateTangents()
{
	const int32 NumVertices = CurrentMeshData.Vertices.Num();
	CurrentMeshData.Tangents.SetNum(NumVertices);

	// Calculate tangents based on UV direction
	for (int32 i = 0; i < NumVertices; ++i)
	{
		const FVector& Normal = CurrentMeshData.Normals[i];

		// Find a tangent perpendicular to the normal
		FVector Tangent;
		if (FMath::Abs(Normal.Z) < 0.9f)
		{
			Tangent = FVector::CrossProduct(FVector::UpVector, Normal).GetSafeNormal();
		}
		else
		{
			Tangent = FVector::CrossProduct(FVector::ForwardVector, Normal).GetSafeNormal();
		}

		CurrentMeshData.Tangents[i] = Tangent;
	}
}
