// ProceduralTreeComponent.h
// Main component for procedural tree generation using L-Systems
// Part of LSystemTrees Plugin - Phase 3

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "Core/LSystem/LSystemTypes.h"
#include "ProceduralTreeComponent.generated.h"

// Forward declarations
class ULSystemGenerator;
class UTurtleInterpreter;
class UTreeGeometry;

// Delegate for tree generation events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTreeGenerated, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTreeGenerationProgress, int32, CurrentStep, int32, TotalSteps);

/**
 * Procedural tree component that generates 3D tree meshes using L-Systems.
 *
 * This component combines L-System string generation, turtle interpretation,
 * and mesh geometry to create procedural trees with branches and leaves.
 *
 * Pipeline:
 *   1. LSystemGenerator: Creates L-System string from axiom + rules
 *   2. TurtleInterpreter: Converts string to branch segments + leaf positions
 *   3. TreeGeometry: Generates mesh vertices/triangles with LOD support
 *   4. ProceduralMeshComponent: Renders the final mesh
 *
 * Features:
 *   - Blueprint-configurable L-System rules
 *   - Multiple LOD levels for performance
 *   - Separate materials for bark and leaves
 *   - Real-time regeneration with seed control
 *   - Debug visualization support
 *
 * Example Usage (C++):
 *   UProceduralTreeComponent* Tree = CreateDefaultSubobject<UProceduralTreeComponent>("Tree");
 *   Tree->Axiom = TEXT("F");
 *   Tree->Rules.Add(FLSystemRule(TEXT("F"), TEXT("FF-[-F+F+F]+[+F-F-F]")));
 *   Tree->Iterations = 4;
 *   Tree->GenerateTree();
 *
 * Example Usage (Blueprint):
 *   1. Add ProceduralTreeComponent to Actor
 *   2. Configure Axiom, Rules, Iterations in Details panel
 *   3. Call GenerateTree() on BeginPlay
 */
UCLASS(ClassGroup = (Procedural), meta = (BlueprintSpawnableComponent, DisplayName = "Procedural Tree"))
class LSYSTEMTREES_API UProceduralTreeComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	UProceduralTreeComponent(const FObjectInitializer& ObjectInitializer);

	// ========================================================================
	// L-System Configuration
	// ========================================================================

	/** The initial string (axiom) for the L-System */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|LSystem",
		meta = (DisplayName = "Axiom"))
	FString Axiom;

	/** Production rules for the L-System */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|LSystem",
		meta = (DisplayName = "Rules", TitleProperty = "Predecessor"))
	TArray<FLSystemRule> Rules;

	/** Number of iterations to apply rules */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|LSystem",
		meta = (ClampMin = "0", ClampMax = "12", DisplayName = "Iterations"))
	int32 Iterations;

	/** Random seed for stochastic rules (0 = random each time) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|LSystem",
		meta = (DisplayName = "Random Seed"))
	int32 RandomSeed;

	// ========================================================================
	// Turtle Configuration
	// ========================================================================

	/** Configuration for turtle interpretation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Turtle",
		meta = (DisplayName = "Turtle Config", ShowOnlyInnerProperties))
	FTurtleConfig TurtleConfig;

	// ========================================================================
	// Geometry Configuration
	// ========================================================================

	/** Configuration for geometry generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Geometry",
		meta = (DisplayName = "Geometry Config", ShowOnlyInnerProperties))
	FTreeGeometryConfig GeometryConfig;

	/** LOD levels configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|LOD",
		meta = (DisplayName = "LOD Levels"))
	TArray<FTreeLODLevel> LODLevels;

	/** Whether to automatically generate tree on component creation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Generation",
		meta = (DisplayName = "Generate On Start"))
	bool bGenerateOnStart;

	// ========================================================================
	// Materials
	// ========================================================================

	/** Material for tree bark (applied to mesh section 0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Materials",
		meta = (DisplayName = "Bark Material"))
	UMaterialInterface* BarkMaterial;

	/** Material for leaves (applied to mesh section 1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Materials",
		meta = (DisplayName = "Leaf Material"))
	UMaterialInterface* LeafMaterial;

	// ========================================================================
	// Events
	// ========================================================================

	/** Called when tree generation completes */
	UPROPERTY(BlueprintAssignable, Category = "Tree|Events")
	FOnTreeGenerated OnTreeGenerated;

	/** Called during tree generation to report progress */
	UPROPERTY(BlueprintAssignable, Category = "Tree|Events")
	FOnTreeGenerationProgress OnGenerationProgress;

	// ========================================================================
	// Generation Methods
	// ========================================================================

	/**
	 * Generate the tree mesh using current settings.
	 * This will clear any existing mesh and create a new one.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tree|Generation",
		meta = (DisplayName = "Generate Tree"))
	void GenerateTree();

	/**
	 * Regenerate the tree with a specific random seed.
	 * @param Seed Random seed for reproducible results
	 */
	UFUNCTION(BlueprintCallable, Category = "Tree|Generation",
		meta = (DisplayName = "Regenerate With Seed"))
	void RegenerateWithSeed(int32 Seed);

	/**
	 * Clear the current tree mesh.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tree|Generation",
		meta = (DisplayName = "Clear Tree"))
	void ClearTree();

	// ========================================================================
	// LOD Control
	// ========================================================================

	/**
	 * Set the active LOD level manually.
	 * @param LODIndex Index of LOD level to display (0 = highest detail)
	 */
	UFUNCTION(BlueprintCallable, Category = "Tree|LOD",
		meta = (DisplayName = "Set LOD Level"))
	void SetLODLevel(int32 LODIndex);

	/**
	 * Get the currently active LOD level.
	 * @return Current LOD index
	 */
	UFUNCTION(BlueprintPure, Category = "Tree|LOD",
		meta = (DisplayName = "Get Current LOD Level"))
	int32 GetCurrentLODLevel() const;

	/**
	 * Get the number of available LOD levels.
	 * @return Number of cached LOD meshes
	 */
	UFUNCTION(BlueprintPure, Category = "Tree|LOD",
		meta = (DisplayName = "Get LOD Count"))
	int32 GetLODCount() const;

	// ========================================================================
	// Statistics
	// ========================================================================

	/**
	 * Get the generated L-System string.
	 * @return The L-System string (can be very long!)
	 */
	UFUNCTION(BlueprintPure, Category = "Tree|Statistics")
	FString GetLSystemString() const;

	/**
	 * Get the number of branch segments generated.
	 * @return Number of branch segments
	 */
	UFUNCTION(BlueprintPure, Category = "Tree|Statistics")
	int32 GetBranchSegmentCount() const;

	/**
	 * Get the number of leaves generated.
	 * @return Number of leaves
	 */
	UFUNCTION(BlueprintPure, Category = "Tree|Statistics")
	int32 GetLeafCount() const;

	/**
	 * Get vertex count for current LOD.
	 * @return Number of vertices in current mesh
	 */
	UFUNCTION(BlueprintPure, Category = "Tree|Statistics")
	int32 GetVertexCount() const;

	/**
	 * Get triangle count for current LOD.
	 * @return Number of triangles in current mesh
	 */
	UFUNCTION(BlueprintPure, Category = "Tree|Statistics")
	int32 GetTriangleCount() const;

	// ========================================================================
	// Debug
	// ========================================================================

	/**
	 * Draw debug visualization of the tree structure.
	 * @param Duration How long to show the debug lines (0 = one frame)
	 */
	UFUNCTION(BlueprintCallable, Category = "Tree|Debug",
		meta = (DevelopmentOnly))
	void DrawDebug(float Duration = 5.0f);

protected:
	// ========================================================================
	// UActorComponent Interface
	// ========================================================================

	virtual void OnComponentCreated() override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// ========================================================================
	// Internal Methods
	// ========================================================================

	/** Initialize default LOD levels if not configured */
	void InitializeDefaultLODs();

	/** Create and configure internal generator objects */
	void InitializeGenerators();

	/** Apply mesh data to the procedural mesh component */
	void ApplyMeshData(const FTreeMeshData& MeshData);

	/** Apply materials to mesh sections */
	void ApplyMaterials();

private:
	// ========================================================================
	// Internal State
	// ========================================================================

	/** L-System generator instance */
	UPROPERTY()
	ULSystemGenerator* Generator;

	/** Turtle interpreter instance */
	UPROPERTY()
	UTurtleInterpreter* Interpreter;

	/** Geometry builder instance */
	UPROPERTY()
	UTreeGeometry* GeometryBuilder;

	/** Cached LOD mesh data */
	TArray<FTreeMeshData> CachedLODs;

	/** Currently displayed LOD index */
	int32 CurrentLODIndex;

	/** Cached L-System string */
	FString CachedLSystemString;

	/** Cached branch segments */
	TArray<FBranchSegment> CachedSegments;

	/** Cached leaf data */
	TArray<FLeafData> CachedLeaves;
};
