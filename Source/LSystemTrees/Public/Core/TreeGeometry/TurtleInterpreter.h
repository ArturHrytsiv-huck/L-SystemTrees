// TurtleInterpreter.h
// Interprets L-System strings as 3D turtle graphics commands
// Part of LSystemTrees Plugin - Phase 3

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/LSystem/LSystemTypes.h"
#include "TurtleInterpreter.generated.h"

// Forward declarations
class UTreeMath;

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogTurtle, Log, All);

/**
 * Interprets L-System strings as 3D turtle graphics commands.
 *
 * The turtle maintains a position, orientation (Forward/Left/Up basis), and width.
 * As it interprets the L-System string character by character, it generates
 * branch segments and leaf placements that can be converted to 3D geometry.
 *
 * Supported Symbols:
 *   F  - Move forward, drawing a branch segment
 *   f  - Move forward without drawing
 *   +  - Rotate right (yaw) by DefaultAngle
 *   -  - Rotate left (yaw) by DefaultAngle
 *   ^  - Pitch up by PitchAngle
 *   &  - Pitch down by PitchAngle
 *   \  - Roll right by RollAngle
 *   /  - Roll left by RollAngle
 *   |  - Turn around (180 degree yaw)
 *   [  - Push current state onto stack (start branch)
 *   ]  - Pop state from stack (end branch)
 *   L  - Place a leaf at current position
 *   X,Y,Z - Ignored (used as placeholders in L-System rules)
 *
 * Example Usage:
 *   UTurtleInterpreter* Interpreter = NewObject<UTurtleInterpreter>();
 *   FTurtleConfig Config;
 *   TArray<FBranchSegment> Segments;
 *   TArray<FLeafData> Leaves;
 *   Interpreter->InterpretString(TEXT("F[+F]F[-F]F"), Config, Segments, Leaves);
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Turtle Interpreter"))
class LSYSTEMTREES_API UTurtleInterpreter : public UObject
{
	GENERATED_BODY()

public:
	UTurtleInterpreter();

	// ========================================================================
	// Main Interpretation
	// ========================================================================

	/**
	 * Interpret an L-System string and generate branch segments and leaves.
	 * @param LSystemString The L-System string to interpret
	 * @param Config Configuration for interpretation (angles, step length, etc.)
	 * @param OutSegments Output array of branch segments
	 * @param OutLeaves Output array of leaf placements
	 */
	UFUNCTION(BlueprintCallable, Category = "Turtle|Interpretation",
		meta = (DisplayName = "Interpret String"))
	void InterpretString(const FString& LSystemString,
	                     const FTurtleConfig& Config,
	                     TArray<FBranchSegment>& OutSegments,
	                     TArray<FLeafData>& OutLeaves);

	/**
	 * Interpret string and return only segments (convenience function).
	 * @param LSystemString The L-System string to interpret
	 * @param Config Configuration for interpretation
	 * @return Array of generated branch segments
	 */
	UFUNCTION(BlueprintCallable, Category = "Turtle|Interpretation")
	TArray<FBranchSegment> InterpretToSegments(const FString& LSystemString,
	                                            const FTurtleConfig& Config);

	// ========================================================================
	// State Access
	// ========================================================================

	/**
	 * Get the current turtle state.
	 * @return Current turtle state (position, orientation, width)
	 */
	UFUNCTION(BlueprintPure, Category = "Turtle|State")
	FTurtleState GetCurrentState() const { return CurrentState; }

	/**
	 * Get the number of segments generated.
	 * @return Number of branch segments
	 */
	UFUNCTION(BlueprintPure, Category = "Turtle|State")
	int32 GetSegmentCount() const { return OutputSegments.Num(); }

	/**
	 * Get the number of leaves generated.
	 * @return Number of leaves
	 */
	UFUNCTION(BlueprintPure, Category = "Turtle|State")
	int32 GetLeafCount() const { return OutputLeaves.Num(); }

	/**
	 * Get the maximum depth reached during interpretation.
	 * @return Maximum branching depth
	 */
	UFUNCTION(BlueprintPure, Category = "Turtle|State")
	int32 GetMaxDepth() const { return MaxDepthReached; }

protected:
	// ========================================================================
	// Symbol Handlers
	// ========================================================================

	/** Handle F/f - Move forward (optionally drawing) */
	void HandleForward(bool bDraw);

	/** Handle +/- - Rotate around Up axis (yaw) */
	void HandleRotateYaw(float AngleDegrees);

	/** Handle ^/& - Rotate around Left axis (pitch) */
	void HandleRotatePitch(float AngleDegrees);

	/** Handle \/\ - Rotate around Forward axis (roll) */
	void HandleRotateRoll(float AngleDegrees);

	/** Handle [ - Push state onto stack */
	void HandlePushState();

	/** Handle ] - Pop state from stack */
	void HandlePopState();

	/** Handle | - Turn around 180 degrees */
	void HandleTurnAround();

	/** Handle L - Place a leaf */
	void HandlePlaceLeaf();

	// ========================================================================
	// Internal Methods
	// ========================================================================

	/** Initialize turtle state from config */
	void InitializeState(const FTurtleConfig& Config);

	/** Reset all internal state */
	void Reset();

	/** Apply tropism to current direction */
	void ApplyTropism();

	/** Ensure basis vectors remain orthonormal */
	void ReorthogonalizeBasis();

	/** Rotate a vector around an axis */
	FVector RotateVector(const FVector& Vector, const FVector& Axis, float AngleDegrees);

	/** Process a single character */
	void ProcessSymbol(TCHAR Symbol);

private:
	// ========================================================================
	// State
	// ========================================================================

	/** Current turtle state */
	FTurtleState CurrentState;

	/** Stack of saved states for branching */
	TArray<FTurtleState> StateStack;

	/** Active configuration */
	FTurtleConfig ActiveConfig;

	/** Random stream for reproducibility */
	FRandomStream RandomStream;

	// ========================================================================
	// Output
	// ========================================================================

	/** Generated branch segments */
	TArray<FBranchSegment> OutputSegments;

	/** Generated leaf placements */
	TArray<FLeafData> OutputLeaves;

	// ========================================================================
	// Statistics
	// ========================================================================

	/** Maximum depth reached during interpretation */
	int32 MaxDepthReached;

	/** Total symbols processed */
	int32 SymbolsProcessed;
};
