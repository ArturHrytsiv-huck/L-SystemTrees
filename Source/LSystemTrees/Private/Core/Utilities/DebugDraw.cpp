// DebugDraw.cpp
// Debug visualization utilities for L-System trees
// Part of LSystemTrees Plugin - Phase 3

#include "Core/Utilities/DebugDraw.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

// ============================================================================
// Branch Visualization
// ============================================================================

void UTreeDebugDraw::DrawBranchSegments(const UObject* WorldContext,
                                         const TArray<FBranchSegment>& Segments,
                                         float Duration,
                                         bool bShowRadius)
{
	if (!WorldContext)
	{
		return;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return;
	}

	// Find max depth for color normalization
	int32 MaxDepth = 0;
	for (const FBranchSegment& Segment : Segments)
	{
		MaxDepth = FMath::Max(MaxDepth, Segment.Depth);
	}

	// Draw each segment
	for (const FBranchSegment& Segment : Segments)
	{
		const FLinearColor LinearColor = GetDepthColor(Segment.Depth, MaxDepth);
		const FColor Color = LinearColor.ToFColor(true);

		// Draw main segment line with thickness based on radius
		const float Thickness = FMath::Max(1.0f, Segment.StartRadius * 0.5f);
		DrawDebugLine(World, Segment.StartPosition, Segment.EndPosition,
		              Color, false, Duration, 0, Thickness);

		// Optionally draw radius circles at joints
		if (bShowRadius)
		{
			// Draw circle at start
			DrawDebugCircle(World, Segment.StartPosition, Segment.StartRadius,
			                16, Color, false, Duration, 0, 1.0f,
			                FVector::RightVector, FVector::ForwardVector, false);

			// Draw circle at end
			DrawDebugCircle(World, Segment.EndPosition, Segment.EndRadius,
			                16, Color, false, Duration, 0, 1.0f,
			                FVector::RightVector, FVector::ForwardVector, false);
		}
	}
}

void UTreeDebugDraw::DrawTurtlePath(const UObject* WorldContext,
                                     const TArray<FBranchSegment>& Segments,
                                     bool bShowOrientation,
                                     float Duration)
{
	if (!WorldContext)
	{
		return;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return;
	}

	const float OrientationLength = 5.0f;

	for (const FBranchSegment& Segment : Segments)
	{
		// Draw segment in white
		DrawDebugLine(World, Segment.StartPosition, Segment.EndPosition,
		              FColor::White, false, Duration, 0, 1.0f);

		// Draw orientation vectors at start of each segment
		if (bShowOrientation)
		{
			const FVector& Pos = Segment.StartPosition;
			const FVector& Forward = Segment.Direction;

			// Calculate Left and Up from Forward
			FVector Left, Up;
			if (FMath::Abs(Forward.Z) < 0.9f)
			{
				Left = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
			}
			else
			{
				Left = FVector::CrossProduct(FVector::ForwardVector, Forward).GetSafeNormal();
			}
			Up = FVector::CrossProduct(Forward, Left).GetSafeNormal();

			// Forward = Red
			DrawDebugLine(World, Pos, Pos + Forward * OrientationLength,
			              FColor::Red, false, Duration, 0, 2.0f);

			// Left = Green
			DrawDebugLine(World, Pos, Pos + Left * OrientationLength,
			              FColor::Green, false, Duration, 0, 2.0f);

			// Up = Blue
			DrawDebugLine(World, Pos, Pos + Up * OrientationLength,
			              FColor::Blue, false, Duration, 0, 2.0f);
		}

		// Draw point at segment start
		DrawDebugPoint(World, Segment.StartPosition, 5.0f, FColor::Yellow, false, Duration);
	}
}

void UTreeDebugDraw::DrawLeaves(const UObject* WorldContext,
                                 const TArray<FLeafData>& Leaves,
                                 float Duration)
{
	if (!WorldContext)
	{
		return;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return;
	}

	const FColor LeafColor = FColor::Green;
	const float NormalLength = 10.0f;

	for (const FLeafData& Leaf : Leaves)
	{
		// Draw leaf position as a point
		DrawDebugPoint(World, Leaf.Position, 8.0f, LeafColor, false, Duration);

		// Draw leaf normal
		DrawDebugLine(World, Leaf.Position, Leaf.Position + Leaf.Normal * NormalLength,
		              FColor::Cyan, false, Duration, 0, 1.0f);

		// Draw a small quad outline representing the leaf
		FVector Right, Up;
		Up = Leaf.UpDirection - Leaf.Normal * FVector::DotProduct(Leaf.UpDirection, Leaf.Normal);
		if (Up.IsNearlyZero())
		{
			if (FMath::Abs(Leaf.Normal.Z) < 0.9f)
			{
				Up = FVector::CrossProduct(Leaf.Normal, FVector::UpVector).GetSafeNormal();
				Up = FVector::CrossProduct(Up, Leaf.Normal);
			}
			else
			{
				Up = FVector::CrossProduct(Leaf.Normal, FVector::ForwardVector).GetSafeNormal();
				Up = FVector::CrossProduct(Up, Leaf.Normal);
			}
		}
		Up.Normalize();
		Right = FVector::CrossProduct(Leaf.Normal, Up).GetSafeNormal();

		const float HalfWidth = Leaf.Size.X * 0.5f;
		const float HalfHeight = Leaf.Size.Y * 0.5f;

		// Quad corners
		const FVector BL = Leaf.Position - Right * HalfWidth - Up * HalfHeight;
		const FVector BR = Leaf.Position + Right * HalfWidth - Up * HalfHeight;
		const FVector TR = Leaf.Position + Right * HalfWidth + Up * HalfHeight;
		const FVector TL = Leaf.Position - Right * HalfWidth + Up * HalfHeight;

		// Draw quad outline
		DrawDebugLine(World, BL, BR, LeafColor, false, Duration, 0, 1.0f);
		DrawDebugLine(World, BR, TR, LeafColor, false, Duration, 0, 1.0f);
		DrawDebugLine(World, TR, TL, LeafColor, false, Duration, 0, 1.0f);
		DrawDebugLine(World, TL, BL, LeafColor, false, Duration, 0, 1.0f);
	}
}

// ============================================================================
// L-System String Visualization
// ============================================================================

void UTreeDebugDraw::PrintLSystemString(const FString& LSystemString, int32 MaxLength)
{
	FString OutputString = LSystemString;

	if (MaxLength > 0 && OutputString.Len() > MaxLength)
	{
		OutputString = OutputString.Left(MaxLength) + TEXT("...");
	}

	// Count symbols
	int32 ForwardCount = 0;
	int32 BranchCount = 0;
	int32 RotationCount = 0;
	int32 LeafCount = 0;

	for (TCHAR c : LSystemString)
	{
		switch (c)
		{
		case TEXT('F'): ForwardCount++; break;
		case TEXT('['): BranchCount++; break;
		case TEXT('+'): case TEXT('-'): case TEXT('^'):
		case TEXT('&'): case TEXT('\\'): case TEXT('/'): RotationCount++; break;
		case TEXT('L'): LeafCount++; break;
		default: break;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("=== L-System String ==="));
	UE_LOG(LogTemp, Log, TEXT("Length: %d characters"), LSystemString.Len());
	UE_LOG(LogTemp, Log, TEXT("Forward (F): %d"), ForwardCount);
	UE_LOG(LogTemp, Log, TEXT("Branches ([): %d"), BranchCount);
	UE_LOG(LogTemp, Log, TEXT("Rotations: %d"), RotationCount);
	UE_LOG(LogTemp, Log, TEXT("Leaves (L): %d"), LeafCount);
	UE_LOG(LogTemp, Log, TEXT("String: %s"), *OutputString);
	UE_LOG(LogTemp, Log, TEXT("======================="));
}

void UTreeDebugDraw::GetLSystemStats(const FString& LSystemString,
                                      int32& OutTotalSymbols,
                                      int32& OutForwardCount,
                                      int32& OutBranchCount,
                                      int32& OutMaxDepth)
{
	OutTotalSymbols = LSystemString.Len();
	OutForwardCount = 0;
	OutBranchCount = 0;
	OutMaxDepth = 0;

	int32 CurrentDepth = 0;

	for (TCHAR c : LSystemString)
	{
		switch (c)
		{
		case TEXT('F'):
			OutForwardCount++;
			break;
		case TEXT('['):
			OutBranchCount++;
			CurrentDepth++;
			OutMaxDepth = FMath::Max(OutMaxDepth, CurrentDepth);
			break;
		case TEXT(']'):
			CurrentDepth = FMath::Max(0, CurrentDepth - 1);
			break;
		default:
			break;
		}
	}
}

// ============================================================================
// Mesh Debug
// ============================================================================

void UTreeDebugDraw::DrawMeshWireframe(const UObject* WorldContext,
                                        const FTreeMeshData& MeshData,
                                        const FTransform& Transform,
                                        float Duration)
{
	if (!WorldContext)
	{
		return;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return;
	}

	const FColor WireColor = FColor::Cyan;

	// Draw each triangle as wireframe
	const int32 NumTriangles = MeshData.Triangles.Num() / 3;
	for (int32 i = 0; i < NumTriangles; ++i)
	{
		const int32 BaseIndex = i * 3;
		if (BaseIndex + 2 >= MeshData.Triangles.Num())
		{
			break;
		}

		const int32 I0 = MeshData.Triangles[BaseIndex];
		const int32 I1 = MeshData.Triangles[BaseIndex + 1];
		const int32 I2 = MeshData.Triangles[BaseIndex + 2];

		if (I0 >= MeshData.Vertices.Num() || I1 >= MeshData.Vertices.Num() || I2 >= MeshData.Vertices.Num())
		{
			continue;
		}

		const FVector V0 = Transform.TransformPosition(MeshData.Vertices[I0]);
		const FVector V1 = Transform.TransformPosition(MeshData.Vertices[I1]);
		const FVector V2 = Transform.TransformPosition(MeshData.Vertices[I2]);

		DrawDebugLine(World, V0, V1, WireColor, false, Duration, 0, 0.5f);
		DrawDebugLine(World, V1, V2, WireColor, false, Duration, 0, 0.5f);
		DrawDebugLine(World, V2, V0, WireColor, false, Duration, 0, 0.5f);
	}
}

void UTreeDebugDraw::DrawMeshNormals(const UObject* WorldContext,
                                      const FTreeMeshData& MeshData,
                                      const FTransform& Transform,
                                      float NormalLength,
                                      float Duration)
{
	if (!WorldContext)
	{
		return;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return;
	}

	const FColor NormalColor = FColor::Blue;

	// Draw normal at each vertex
	const int32 NumVertices = FMath::Min(MeshData.Vertices.Num(), MeshData.Normals.Num());
	for (int32 i = 0; i < NumVertices; ++i)
	{
		const FVector Position = Transform.TransformPosition(MeshData.Vertices[i]);
		const FVector Normal = Transform.TransformVector(MeshData.Normals[i]).GetSafeNormal();
		const FVector NormalEnd = Position + Normal * NormalLength;

		DrawDebugLine(World, Position, NormalEnd, NormalColor, false, Duration, 0, 0.5f);
	}
}

// ============================================================================
// Utility
// ============================================================================

FLinearColor UTreeDebugDraw::GetDepthColor(int32 Depth, int32 MaxDepth)
{
	// Interpolate from brown (trunk) to green (tips)
	const FLinearColor TrunkColor(0.4f, 0.26f, 0.13f, 1.0f); // Brown
	const FLinearColor TipColor(0.2f, 0.8f, 0.2f, 1.0f);      // Green

	if (MaxDepth <= 0)
	{
		return TrunkColor;
	}

	const float T = FMath::Clamp(static_cast<float>(Depth) / static_cast<float>(MaxDepth), 0.0f, 1.0f);
	return FLinearColor::LerpUsingHSV(TrunkColor, TipColor, T);
}

FLinearColor UTreeDebugDraw::GetSymbolColor(const FString& Symbol)
{
	if (Symbol.Len() == 0)
	{
		return FLinearColor::White;
	}

	const TCHAR c = Symbol[0];

	switch (c)
	{
	// Movement - Yellow
	case TEXT('F'):
	case TEXT('f'):
		return FLinearColor::Yellow;

	// Rotation - Cyan
	case TEXT('+'):
	case TEXT('-'):
	case TEXT('^'):
	case TEXT('&'):
	case TEXT('\\'):
	case TEXT('/'):
	case TEXT('|'):
		return FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);

	// Branching - Magenta
	case TEXT('['):
	case TEXT(']'):
		return FLinearColor(1.0f, 0.0f, 1.0f, 1.0f);

	// Leaves - Green
	case TEXT('L'):
		return FLinearColor::Green;

	// Variables - Gray
	case TEXT('X'):
	case TEXT('Y'):
	case TEXT('Z'):
	case TEXT('A'):
	case TEXT('B'):
		return FLinearColor::Gray;

	default:
		return FLinearColor::White;
	}
}
