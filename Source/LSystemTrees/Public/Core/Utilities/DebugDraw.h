// DebugDraw.h
// Debug visualization utilities for L-System trees
// Part of LSystemTrees Plugin - Phase 3

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/LSystem/LSystemTypes.h"
#include "DebugDraw.generated.h"

/**
 * Debug visualization utilities for L-System tree generation.
 *
 * Provides functions to visualize branch segments, turtle paths, and L-System
 * strings for debugging and development purposes.
 *
 * All drawing functions use Unreal's debug draw system which renders in the
 * game viewport when debugging is enabled.
 *
 * Example Usage:
 *   UTreeDebugDraw::DrawBranchSegments(GetWorld(), Segments, 10.0f);
 *   UTreeDebugDraw::DrawTurtlePath(GetWorld(), Segments, true);
 */
UCLASS()
class LSYSTEMTREES_API UTreeDebugDraw : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ========================================================================
	// Branch Visualization
	// ========================================================================

	/**
	 * Draw branch segments as debug lines with width indication.
	 * @param WorldContext World context for drawing
	 * @param Segments Array of branch segments to visualize
	 * @param Duration How long the debug lines persist (0 = one frame)
	 * @param bShowRadius Whether to show radius circles at joints
	 */
	UFUNCTION(BlueprintCallable, Category = "TreeDebug|Visualization",
		meta = (WorldContext = "WorldContext", DevelopmentOnly))
	static void DrawBranchSegments(const UObject* WorldContext,
	                               const TArray<FBranchSegment>& Segments,
	                               float Duration = 5.0f,
	                               bool bShowRadius = false);

	/**
	 * Draw turtle path with orientation indicators.
	 * Shows Forward (Red), Left (Green), Up (Blue) vectors at each segment start.
	 * @param WorldContext World context for drawing
	 * @param Segments Array of branch segments to visualize
	 * @param bShowOrientation Whether to show orientation vectors
	 * @param Duration How long the debug lines persist
	 */
	UFUNCTION(BlueprintCallable, Category = "TreeDebug|Visualization",
		meta = (WorldContext = "WorldContext", DevelopmentOnly))
	static void DrawTurtlePath(const UObject* WorldContext,
	                           const TArray<FBranchSegment>& Segments,
	                           bool bShowOrientation = true,
	                           float Duration = 5.0f);

	/**
	 * Draw leaf positions as debug markers.
	 * @param WorldContext World context for drawing
	 * @param Leaves Array of leaf data to visualize
	 * @param Duration How long the debug markers persist
	 */
	UFUNCTION(BlueprintCallable, Category = "TreeDebug|Visualization",
		meta = (WorldContext = "WorldContext", DevelopmentOnly))
	static void DrawLeaves(const UObject* WorldContext,
	                       const TArray<FLeafData>& Leaves,
	                       float Duration = 5.0f);

	// ========================================================================
	// L-System String Visualization
	// ========================================================================

	/**
	 * Print L-System string to output log with symbol highlighting.
	 * @param LSystemString The L-System string to print
	 * @param MaxLength Maximum characters to print (0 = unlimited)
	 */
	UFUNCTION(BlueprintCallable, Category = "TreeDebug|Output")
	static void PrintLSystemString(const FString& LSystemString,
	                               int32 MaxLength = 200);

	/**
	 * Get statistics about an L-System string.
	 * @param LSystemString The L-System string to analyze
	 * @param OutTotalSymbols Total number of symbols
	 * @param OutForwardCount Number of F symbols (drawing moves)
	 * @param OutBranchCount Number of [ symbols (branches)
	 * @param OutMaxDepth Maximum nesting depth
	 */
	UFUNCTION(BlueprintCallable, Category = "TreeDebug|Analysis")
	static void GetLSystemStats(const FString& LSystemString,
	                            int32& OutTotalSymbols,
	                            int32& OutForwardCount,
	                            int32& OutBranchCount,
	                            int32& OutMaxDepth);

	// ========================================================================
	// Mesh Debug
	// ========================================================================

	/**
	 * Draw mesh wireframe for debugging geometry.
	 * @param WorldContext World context for drawing
	 * @param MeshData Mesh data to visualize
	 * @param Transform Transform to apply to vertices
	 * @param Duration How long the wireframe persists
	 */
	UFUNCTION(BlueprintCallable, Category = "TreeDebug|Mesh",
		meta = (WorldContext = "WorldContext", DevelopmentOnly))
	static void DrawMeshWireframe(const UObject* WorldContext,
	                              const FTreeMeshData& MeshData,
	                              const FTransform& Transform,
	                              float Duration = 5.0f);

	/**
	 * Draw mesh normals for debugging.
	 * @param WorldContext World context for drawing
	 * @param MeshData Mesh data to visualize
	 * @param Transform Transform to apply to vertices
	 * @param NormalLength Length of normal lines
	 * @param Duration How long the normals persist
	 */
	UFUNCTION(BlueprintCallable, Category = "TreeDebug|Mesh",
		meta = (WorldContext = "WorldContext", DevelopmentOnly))
	static void DrawMeshNormals(const UObject* WorldContext,
	                            const FTreeMeshData& MeshData,
	                            const FTransform& Transform,
	                            float NormalLength = 5.0f,
	                            float Duration = 5.0f);

	// ========================================================================
	// Utility
	// ========================================================================

	/**
	 * Get a color based on branch depth.
	 * @param Depth Branch depth (0 = trunk)
	 * @param MaxDepth Maximum expected depth for normalization
	 * @return Color from trunk (brown) to tip (green)
	 */
	UFUNCTION(BlueprintPure, Category = "TreeDebug|Utility")
	static FLinearColor GetDepthColor(int32 Depth, int32 MaxDepth = 5);

	/**
	 * Get a color for an L-System symbol category.
	 * @param Symbol The L-System symbol
	 * @return Color based on symbol type
	 */
	UFUNCTION(BlueprintPure, Category = "TreeDebug|Utility")
	static FLinearColor GetSymbolColor(const FString& Symbol);
};
