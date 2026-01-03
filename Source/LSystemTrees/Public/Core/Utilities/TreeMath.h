// TreeMath.h
// Mathematical utilities for tree geometry and turtle interpretation
// Part of LSystemTrees Plugin - Phase 3

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TreeMath.generated.h"

/**
 * Blueprint function library providing mathematical utilities for tree generation.
 * Includes vector rotation, basis orthogonalization, and tree-specific calculations.
 */
UCLASS()
class LSYSTEMTREES_API UTreeMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ========================================================================
	// Vector Rotation
	// ========================================================================

	/**
	 * Rotate a vector around an arbitrary axis by a given angle.
	 * Uses Rodrigues' rotation formula.
	 * @param Vector The vector to rotate
	 * @param Axis The axis to rotate around (will be normalized)
	 * @param AngleDegrees Rotation angle in degrees
	 * @return The rotated vector
	 */
	UFUNCTION(BlueprintPure, Category = "TreeMath|Rotation",
		meta = (DisplayName = "Rotate Vector Around Axis"))
	static FVector RotateVectorAroundAxis(const FVector& Vector, const FVector& Axis, float AngleDegrees);

	/**
	 * Rotate a vector around the X axis.
	 * @param Vector The vector to rotate
	 * @param AngleDegrees Rotation angle in degrees
	 * @return The rotated vector
	 */
	UFUNCTION(BlueprintPure, Category = "TreeMath|Rotation")
	static FVector RotateAroundX(const FVector& Vector, float AngleDegrees);

	/**
	 * Rotate a vector around the Y axis.
	 * @param Vector The vector to rotate
	 * @param AngleDegrees Rotation angle in degrees
	 * @return The rotated vector
	 */
	UFUNCTION(BlueprintPure, Category = "TreeMath|Rotation")
	static FVector RotateAroundY(const FVector& Vector, float AngleDegrees);

	/**
	 * Rotate a vector around the Z axis.
	 * @param Vector The vector to rotate
	 * @param AngleDegrees Rotation angle in degrees
	 * @return The rotated vector
	 */
	UFUNCTION(BlueprintPure, Category = "TreeMath|Rotation")
	static FVector RotateAroundZ(const FVector& Vector, float AngleDegrees);

	// ========================================================================
	// Basis Vector Operations
	// ========================================================================

	/**
	 * Reorthogonalize a basis to ensure vectors are perpendicular and normalized.
	 * Uses Gram-Schmidt orthogonalization with Forward as the primary vector.
	 * @param Forward Primary direction (will be normalized, others adjusted to be perpendicular)
	 * @param Left Will be adjusted to be perpendicular to Forward
	 * @param Up Will be recalculated as cross product of Forward and Left
	 */
	UFUNCTION(BlueprintCallable, Category = "TreeMath|Basis")
	static void ReorthogonalizeBasis(UPARAM(ref) FVector& Forward, UPARAM(ref) FVector& Left, UPARAM(ref) FVector& Up);

	/**
	 * Calculate perpendicular vectors for a given direction.
	 * Useful for generating cylinder geometry around a branch.
	 * @param Direction The direction to find perpendicular vectors for
	 * @param OutRight Output: A vector perpendicular to Direction
	 * @param OutUp Output: A vector perpendicular to both Direction and OutRight
	 */
	UFUNCTION(BlueprintCallable, Category = "TreeMath|Basis")
	static void GetPerpendicularVectors(const FVector& Direction, FVector& OutRight, FVector& OutUp);

	/**
	 * Create a rotation quaternion from a forward and up vector.
	 * @param Forward The forward direction
	 * @param Up The up direction
	 * @return Quaternion representing the orientation
	 */
	UFUNCTION(BlueprintPure, Category = "TreeMath|Basis")
	static FQuat MakeQuatFromBasis(const FVector& Forward, const FVector& Up);

	// ========================================================================
	// Tree-Specific Calculations
	// ========================================================================

	/**
	 * Calculate child branch width using Leonardo's rule.
	 * Leonardo's rule: parent_width^n = child1_width^n + child2_width^n + ...
	 * This ensures realistic branch proportions where the cross-sectional area is conserved.
	 * @param ParentWidth Width of the parent branch
	 * @param ChildCount Number of child branches (default 2)
	 * @param Exponent Leonardo exponent (typically 2.0 for area conservation)
	 * @return Calculated width for each child branch
	 */
	UFUNCTION(BlueprintPure, Category = "TreeMath|Branch",
		meta = (DisplayName = "Calculate Child Width (Leonardo Rule)"))
	static float CalculateChildWidth(float ParentWidth, int32 ChildCount = 2, float Exponent = 2.0f);

	/**
	 * Calculate branch width based on depth with linear falloff.
	 * @param InitialWidth Starting width (trunk)
	 * @param Depth Current branching depth
	 * @param FalloffRate Rate of width reduction per depth level (0-1)
	 * @param MinWidth Minimum width to clamp to
	 * @return Calculated width at the given depth
	 */
	UFUNCTION(BlueprintPure, Category = "TreeMath|Branch")
	static float CalculateWidthAtDepth(float InitialWidth, int32 Depth, float FalloffRate, float MinWidth);

	/**
	 * Apply tropism (gravity influence) to a direction vector.
	 * Gradually bends the direction toward the tropism vector.
	 * @param CurrentDirection The current direction to modify
	 * @param TropismVector The direction of tropism (e.g., gravity)
	 * @param Strength How strongly to apply tropism (0-1)
	 * @return The modified direction vector (normalized)
	 */
	UFUNCTION(BlueprintPure, Category = "TreeMath|Tropism")
	static FVector ApplyTropism(const FVector& CurrentDirection, const FVector& TropismVector, float Strength);

	// ========================================================================
	// Geometry Helpers
	// ========================================================================

	/**
	 * Calculate a point on a circle in 3D space.
	 * Used for generating cylinder rings.
	 * @param Center Center of the circle
	 * @param Normal Normal to the circle plane (axis of cylinder)
	 * @param Radius Radius of the circle
	 * @param AngleDegrees Angle around the circle (0-360)
	 * @return Point on the circle
	 */
	UFUNCTION(BlueprintPure, Category = "TreeMath|Geometry")
	static FVector GetPointOnCircle(const FVector& Center, const FVector& Normal, float Radius, float AngleDegrees);

	/**
	 * Generate points forming a ring around a center point.
	 * @param Center Center of the ring
	 * @param Normal Normal to the ring plane
	 * @param Radius Radius of the ring
	 * @param NumPoints Number of points to generate
	 * @return Array of points forming the ring
	 */
	UFUNCTION(BlueprintCallable, Category = "TreeMath|Geometry")
	static TArray<FVector> GenerateRingPoints(const FVector& Center, const FVector& Normal, float Radius, int32 NumPoints);

	/**
	 * Calculate the normal vector for a triangle.
	 * @param V0 First vertex
	 * @param V1 Second vertex
	 * @param V2 Third vertex
	 * @return Normal vector (normalized)
	 */
	UFUNCTION(BlueprintPure, Category = "TreeMath|Geometry")
	static FVector CalculateTriangleNormal(const FVector& V0, const FVector& V1, const FVector& V2);

	/**
	 * Interpolate between two radii along a segment.
	 * @param StartRadius Radius at start
	 * @param EndRadius Radius at end
	 * @param T Interpolation factor (0-1)
	 * @return Interpolated radius
	 */
	UFUNCTION(BlueprintPure, Category = "TreeMath|Geometry")
	static float LerpRadius(float StartRadius, float EndRadius, float T);

	// ========================================================================
	// Random Utilities
	// ========================================================================

	/**
	 * Generate a random rotation angle within a range.
	 * @param MaxAngle Maximum angle in degrees
	 * @param RandomStream Random stream for reproducibility
	 * @return Random angle between -MaxAngle and +MaxAngle
	 */
	static float RandomAngle(float MaxAngle, FRandomStream& RandomStream);

	/**
	 * Generate a random direction within a cone.
	 * @param ConeAxis Central axis of the cone
	 * @param ConeAngle Half-angle of the cone in degrees
	 * @param RandomStream Random stream for reproducibility
	 * @return Random direction within the cone
	 */
	static FVector RandomDirectionInCone(const FVector& ConeAxis, float ConeAngle, FRandomStream& RandomStream);
};
