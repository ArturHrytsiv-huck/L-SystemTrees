// TreeGeometry.h
// Generates mesh geometry from branch segments and leaves
// Part of LSystemTrees Plugin - Phase 3

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/LSystem/LSystemTypes.h"
#include "TreeGeometry.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogTreeGeometry, Log, All);

/**
 * Generates mesh geometry from branch segments and leaf placements.
 *
 * Takes the output from TurtleInterpreter (segments and leaves) and converts
 * them into vertex/index data suitable for UProceduralMeshComponent.
 *
 * Features:
 *   - Tapered cylinder generation for branches
 *   - Quad generation for leaves
 *   - Multiple LOD level support
 *   - UV mapping for bark and leaf textures
 *   - Normal and tangent calculation
 *
 * Example Usage:
 *   UTreeGeometry* Geometry = NewObject<UTreeGeometry>();
 *   FTreeGeometryConfig Config;
 *   TArray<FTreeMeshData> LODs = Geometry->GenerateMeshLODs(Segments, Leaves, Config.LODLevels);
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Tree Geometry Generator"))
class LSYSTEMTREES_API UTreeGeometry : public UObject
{
	GENERATED_BODY()

public:
	UTreeGeometry();

	// ========================================================================
	// LOD Generation
	// ========================================================================

	/**
	 * Generate mesh data for all LOD levels.
	 * @param Segments Branch segments from turtle interpretation
	 * @param Leaves Leaf placements from turtle interpretation
	 * @param LODLevels Configuration for each LOD level
	 * @return Array of mesh data, one per LOD level
	 */
	UFUNCTION(BlueprintCallable, Category = "TreeGeometry|Generation")
	TArray<FTreeMeshData> GenerateMeshLODs(const TArray<FBranchSegment>& Segments,
	                                        const TArray<FLeafData>& Leaves,
	                                        const TArray<FTreeLODLevel>& LODLevels);

	/**
	 * Generate mesh data for a single detail level.
	 * @param Segments Branch segments from turtle interpretation
	 * @param Leaves Leaf placements from turtle interpretation
	 * @param RadialSegments Number of segments around each cylinder
	 * @param bIncludeLeaves Whether to include leaf geometry
	 * @return Generated mesh data
	 */
	UFUNCTION(BlueprintCallable, Category = "TreeGeometry|Generation")
	FTreeMeshData GenerateMesh(const TArray<FBranchSegment>& Segments,
	                           const TArray<FLeafData>& Leaves,
	                           int32 RadialSegments,
	                           bool bIncludeLeaves);

	// ========================================================================
	// Configuration
	// ========================================================================

	/** UV tiling factor along branch length */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeGeometry|UVs")
	float BarkUVTiling;

	/** Default leaf size if not specified */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeGeometry|Leaves")
	FVector2D DefaultLeafSize;

protected:
	// ========================================================================
	// Branch Geometry
	// ========================================================================

	/**
	 * Generate a tapered cylinder for a single branch segment.
	 * @param Segment The branch segment to generate geometry for
	 * @param RadialSegments Number of segments around the cylinder
	 */
	void GenerateBranchCylinder(const FBranchSegment& Segment, int32 RadialSegments);

	/**
	 * Generate a tapered cylinder with connectivity to parent segment.
	 * Reuses parent's end ring when connected for smooth joints.
	 * @param Segment The branch segment to generate geometry for
	 * @param SegmentIndex Index of this segment in the array
	 * @param RadialSegments Number of segments around the cylinder
	 */
	void GenerateBranchCylinderConnected(const FBranchSegment& Segment, int32 SegmentIndex, int32 RadialSegments);

	/**
	 * Generate a ring of vertices around a point.
	 * @param Center Center point of the ring
	 * @param Direction Direction the ring faces (cylinder axis)
	 * @param Radius Radius of the ring
	 * @param NumSegments Number of vertices in the ring
	 * @param V UV V coordinate for this ring
	 * @return Index of the first vertex in the ring
	 */
	int32 GenerateRing(const FVector& Center, const FVector& Direction, float Radius,
	                   int32 NumSegments, float V);

	/**
	 * Connect two rings with triangles.
	 * @param StartRingIndex First vertex index of the first ring
	 * @param EndRingIndex First vertex index of the second ring
	 * @param NumSegments Number of segments in each ring
	 */
	void ConnectRings(int32 StartRingIndex, int32 EndRingIndex, int32 NumSegments);

	// ========================================================================
	// Leaf Geometry
	// ========================================================================

	/**
	 * Generate a quad for a single leaf.
	 * @param Leaf The leaf placement data
	 */
	void GenerateLeafQuad(const FLeafData& Leaf);

	// ========================================================================
	// Utility Methods
	// ========================================================================

	/** Reset mesh data for a new generation pass */
	void ResetMeshData();

	/** Calculate perpendicular vectors for a direction */
	void GetPerpendicularVectors(const FVector& Direction, FVector& OutRight, FVector& OutUp);

	/** Calculate smooth normals for the mesh */
	void CalculateSmoothNormals();

	/** Calculate tangent vectors for normal mapping */
	void CalculateTangents();

private:
	/** Current mesh data being built */
	FTreeMeshData CurrentMeshData;

	/** Current accumulated V coordinate for UV mapping */
	float CurrentVCoordinate;

	/** Random stream for leaf rotation variation */
	FRandomStream RandomStream;

	/** Maps segment index to its end ring's first vertex index */
	TMap<int32, int32> SegmentEndRingIndices;

	/** Current radial segments count (cached for connected segments) */
	int32 CurrentRadialSegments;
};
