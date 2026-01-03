// LSystemTypes.h
// Core data structures for L-System string generation
// Part of LSystemTrees Plugin - Phase 2

#pragma once

#include "CoreMinimal.h"
#include "LSystemTypes.generated.h"

// ============================================================================
// FLSystemRule - Production Rule with Context-Sensitive Support
// ============================================================================

/**
 * Represents a single L-System production rule.
 * Supports context-sensitive rules in the format: LeftContext < Predecessor > RightContext -> Successor
 *
 * Examples:
 *   Simple rule:           F -> FF           (FLSystemRule("", "F", "", "FF", 1.0))
 *   Left context:          A < B -> X        (FLSystemRule("A", "B", "", "X", 1.0))
 *   Right context:         B > C -> X        (FLSystemRule("", "B", "C", "X", 1.0))
 *   Full context:          A < B > C -> X    (FLSystemRule("A", "B", "C", "X", 1.0))
 *   Stochastic:            F -> F[+F] (50%)  (FLSystemRule("", "F", "", "F[+F]", 0.5))
 */
USTRUCT(BlueprintType)
struct LSYSTEMTREES_API FLSystemRule
{
	GENERATED_BODY()

	/** Left context - symbol that must precede the predecessor (empty = no left context requirement) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Rule")
	FString LeftContext;

	/** The symbol to replace (predecessor) - must be exactly 1 character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Rule")
	FString Predecessor;

	/** Right context - symbol that must follow the predecessor (empty = no right context requirement) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Rule")
	FString RightContext;

	/** The replacement string (successor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Rule")
	FString Successor;

	/** Probability of this rule being applied (0.0 - 1.0). Used for stochastic L-Systems. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Rule",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Probability;

	// ========== Constructors ==========

	/** Default constructor */
	FLSystemRule()
		: LeftContext(TEXT(""))
		, Predecessor(TEXT(""))
		, RightContext(TEXT(""))
		, Successor(TEXT(""))
		, Probability(1.0f)
	{
	}

	/** Simple rule constructor (context-free) */
	FLSystemRule(const FString& InPredecessor, const FString& InSuccessor, float InProbability = 1.0f)
		: LeftContext(TEXT(""))
		, Predecessor(InPredecessor)
		, RightContext(TEXT(""))
		, Successor(InSuccessor)
		, Probability(FMath::Clamp(InProbability, 0.0f, 1.0f))
	{
	}

	/** Simple rule constructor with TCHAR predecessor */
	FLSystemRule(TCHAR InPredecessor, const FString& InSuccessor, float InProbability = 1.0f)
		: LeftContext(TEXT(""))
		, Predecessor(FString::Chr(InPredecessor))
		, RightContext(TEXT(""))
		, Successor(InSuccessor)
		, Probability(FMath::Clamp(InProbability, 0.0f, 1.0f))
	{
	}

	/** Full context-sensitive constructor */
	FLSystemRule(const FString& InLeftContext, const FString& InPredecessor,
	             const FString& InRightContext, const FString& InSuccessor,
	             float InProbability = 1.0f)
		: LeftContext(InLeftContext)
		, Predecessor(InPredecessor)
		, RightContext(InRightContext)
		, Successor(InSuccessor)
		, Probability(FMath::Clamp(InProbability, 0.0f, 1.0f))
	{
	}

	// ========== Validation ==========

	/** Check if the rule is valid */
	bool IsValid() const
	{
		// Predecessor must be exactly 1 character
		if (Predecessor.Len() != 1)
		{
			return false;
		}

		// Successor cannot be empty
		if (Successor.Len() == 0)
		{
			return false;
		}

		// Probability must be in valid range
		if (Probability < 0.0f || Probability > 1.0f)
		{
			return false;
		}

		// Context must be 0 or 1 character
		if (LeftContext.Len() > 1 || RightContext.Len() > 1)
		{
			return false;
		}

		return true;
	}

	/** Validate and return error message if invalid */
	bool Validate(FString& OutError) const
	{
		if (Predecessor.Len() != 1)
		{
			OutError = FString::Printf(TEXT("Predecessor must be exactly 1 character, got '%s' (len=%d)"),
				*Predecessor, Predecessor.Len());
			return false;
		}

		if (Successor.Len() == 0)
		{
			OutError = TEXT("Successor cannot be empty");
			return false;
		}

		if (Probability < 0.0f || Probability > 1.0f)
		{
			OutError = FString::Printf(TEXT("Probability must be between 0 and 1, got %f"), Probability);
			return false;
		}

		if (LeftContext.Len() > 1)
		{
			OutError = FString::Printf(TEXT("Left context must be 0 or 1 character, got '%s'"), *LeftContext);
			return false;
		}

		if (RightContext.Len() > 1)
		{
			OutError = FString::Printf(TEXT("Right context must be 0 or 1 character, got '%s'"), *RightContext);
			return false;
		}

		OutError.Empty();
		return true;
	}

	// ========== Accessors ==========

	/** Get the predecessor as TCHAR */
	TCHAR GetPredecessorChar() const
	{
		return Predecessor.Len() > 0 ? Predecessor[0] : TEXT('\0');
	}

	/** Get the left context as TCHAR (returns '\0' if empty) */
	TCHAR GetLeftContextChar() const
	{
		return LeftContext.Len() > 0 ? LeftContext[0] : TEXT('\0');
	}

	/** Get the right context as TCHAR (returns '\0' if empty) */
	TCHAR GetRightContextChar() const
	{
		return RightContext.Len() > 0 ? RightContext[0] : TEXT('\0');
	}

	/** Check if this is a context-sensitive rule */
	bool IsContextSensitive() const
	{
		return LeftContext.Len() > 0 || RightContext.Len() > 0;
	}

	/** Check if this rule matches the given context */
	bool MatchesContext(TCHAR InLeftContext, TCHAR InRightContext) const
	{
		// Check left context (empty means match any)
		if (LeftContext.Len() > 0 && GetLeftContextChar() != InLeftContext)
		{
			return false;
		}

		// Check right context (empty means match any)
		if (RightContext.Len() > 0 && GetRightContextChar() != InRightContext)
		{
			return false;
		}

		return true;
	}

	/** Get context specificity score (higher = more specific) */
	int32 GetContextSpecificity() const
	{
		int32 Score = 0;
		if (LeftContext.Len() > 0) Score++;
		if (RightContext.Len() > 0) Score++;
		return Score;
	}

	// ========== Utility ==========

	/** Convert to human-readable string */
	FString ToString() const
	{
		FString Result;

		if (LeftContext.Len() > 0)
		{
			Result += LeftContext + TEXT(" < ");
		}

		Result += Predecessor;

		if (RightContext.Len() > 0)
		{
			Result += TEXT(" > ") + RightContext;
		}

		Result += TEXT(" -> ") + Successor;

		if (!FMath::IsNearlyEqual(Probability, 1.0f))
		{
			Result += FString::Printf(TEXT(" (p=%.2f)"), Probability);
		}

		return Result;
	}

	// ========== Operators ==========

	bool operator==(const FLSystemRule& Other) const
	{
		return LeftContext == Other.LeftContext &&
		       Predecessor == Other.Predecessor &&
		       RightContext == Other.RightContext &&
		       Successor == Other.Successor &&
		       FMath::IsNearlyEqual(Probability, Other.Probability);
	}

	bool operator!=(const FLSystemRule& Other) const
	{
		return !(*this == Other);
	}
};

// ============================================================================
// FLSystemConfig - Generator Configuration
// ============================================================================

/**
 * Configuration settings for L-System generation.
 */
USTRUCT(BlueprintType)
struct LSYSTEMTREES_API FLSystemConfig
{
	GENERATED_BODY()

	/** Maximum number of iterations allowed (protection against infinite loops) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Config",
		meta = (ClampMin = "1", ClampMax = "100", UIMin = "1", UIMax = "20"))
	int32 MaxIterations;

	/** Maximum string length before termination (protection against memory overflow) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Config",
		meta = (ClampMin = "1", ClampMax = "10000000", UIMin = "100", UIMax = "1000000"))
	int32 MaxStringLength;

	/** Random seed for reproducible stochastic generation (0 = use time-based seed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Config")
	int32 RandomSeed;

	/** Whether to store iteration history (uses more memory but useful for debugging) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Config")
	bool bStoreHistory;

	/** Whether to enable detailed logging of each iteration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Config")
	bool bEnableDetailedLogging;

	/** Default constructor */
	FLSystemConfig()
		: MaxIterations(10)
		, MaxStringLength(100000)
		, RandomSeed(0)
		, bStoreHistory(true)
		, bEnableDetailedLogging(true)
	{
	}
};

// ============================================================================
// FLSystemStatistics - Generation Statistics
// ============================================================================

/**
 * Statistics about L-System generation process.
 */
USTRUCT(BlueprintType)
struct LSYSTEMTREES_API FLSystemStatistics
{
	GENERATED_BODY()

	/** Total number of iterations performed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|Statistics")
	int32 TotalIterations;

	/** Length of the final generated string */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|Statistics")
	int32 FinalStringLength;

	/** Time taken for generation in milliseconds */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|Statistics")
	float GenerationTimeMs;

	/** Total number of rule applications */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|Statistics")
	int32 RulesApplied;

	/** Number of context-sensitive rule applications */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|Statistics")
	int32 ContextRulesApplied;

	/** Count of each symbol in the final string */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|Statistics")
	TMap<FString, int32> SymbolCounts;

	/** Default constructor */
	FLSystemStatistics()
		: TotalIterations(0)
		, FinalStringLength(0)
		, GenerationTimeMs(0.0f)
		, RulesApplied(0)
		, ContextRulesApplied(0)
	{
	}

	/** Reset all statistics */
	void Reset()
	{
		TotalIterations = 0;
		FinalStringLength = 0;
		GenerationTimeMs = 0.0f;
		RulesApplied = 0;
		ContextRulesApplied = 0;
		SymbolCounts.Empty();
	}

	/** Convert to human-readable string */
	FString ToString() const
	{
		return FString::Printf(
			TEXT("Iterations: %d, Length: %d, Time: %.2fms, Rules: %d (Context: %d)"),
			TotalIterations, FinalStringLength, GenerationTimeMs, RulesApplied, ContextRulesApplied
		);
	}
};

// ============================================================================
// FLSystemState - Current Generation State
// ============================================================================

/**
 * Current state during L-System generation.
 */
USTRUCT(BlueprintType)
struct LSYSTEMTREES_API FLSystemState
{
	GENERATED_BODY()

	/** The current generated string */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|State")
	FString CurrentString;

	/** Current iteration number */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|State")
	int32 CurrentIteration;

	/** History of strings from each iteration */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|State")
	TArray<FString> History;

	/** Whether generation is currently in progress */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|State")
	bool bIsGenerating;

	/** Progress percentage for async generation (0.0 - 1.0) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|State")
	float ProgressPercent;

	/** Default constructor */
	FLSystemState()
		: CurrentString(TEXT(""))
		, CurrentIteration(0)
		, bIsGenerating(false)
		, ProgressPercent(0.0f)
	{
	}

	/** Reset to initial state */
	void Reset()
	{
		CurrentString.Empty();
		CurrentIteration = 0;
		History.Empty();
		bIsGenerating = false;
		ProgressPercent = 0.0f;
	}

	/** Initialize with axiom */
	void Initialize(const FString& Axiom)
	{
		Reset();
		CurrentString = Axiom;
	}
};

// ============================================================================
// FLSystemGenerationResult - Generation Result with Metadata
// ============================================================================

/**
 * Result of L-System generation operation.
 * Contains the generated string, success status, and statistics.
 */
USTRUCT(BlueprintType)
struct LSYSTEMTREES_API FLSystemGenerationResult
{
	GENERATED_BODY()

	/** The final generated string */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|Result")
	FString GeneratedString;

	/** Whether generation was successful */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|Result")
	bool bSuccess;

	/** Error message if generation failed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|Result")
	FString ErrorMessage;

	/** History of strings from each iteration (if enabled in config) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|Result")
	TArray<FString> IterationHistory;

	/** Generation statistics */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LSystem|Result")
	FLSystemStatistics Stats;

	/** Default constructor */
	FLSystemGenerationResult()
		: GeneratedString(TEXT(""))
		, bSuccess(false)
		, ErrorMessage(TEXT(""))
	{
	}

	/** Create a success result */
	static FLSystemGenerationResult Success(const FString& Result,
	                                        const TArray<FString>& History,
	                                        const FLSystemStatistics& Statistics)
	{
		FLSystemGenerationResult Out;
		Out.GeneratedString = Result;
		Out.bSuccess = true;
		Out.ErrorMessage.Empty();
		Out.IterationHistory = History;
		Out.Stats = Statistics;
		return Out;
	}

	/** Create a failure result */
	static FLSystemGenerationResult Failure(const FString& Error)
	{
		FLSystemGenerationResult Out;
		Out.bSuccess = false;
		Out.ErrorMessage = Error;
		return Out;
	}

	/** Create a cancelled result */
	static FLSystemGenerationResult Cancelled()
	{
		FLSystemGenerationResult Out;
		Out.bSuccess = false;
		Out.ErrorMessage = TEXT("Generation was cancelled");
		return Out;
	}
};

// ============================================================================
// FTurtleState - Turtle Graphics State for 3D Interpretation
// ============================================================================

/**
 * Represents the current state of a turtle in 3D space.
 * Used by TurtleInterpreter to track position and orientation during L-System interpretation.
 */
USTRUCT(BlueprintType)
struct LSYSTEMTREES_API FTurtleState
{
	GENERATED_BODY()

	/** Current 3D position of the turtle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|State")
	FVector Position;

	/** Forward direction (heading) - normalized */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|State")
	FVector Forward;

	/** Left direction - normalized, perpendicular to Forward and Up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|State")
	FVector Left;

	/** Up direction - normalized */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|State")
	FVector Up;

	/** Current branch width/radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|State")
	float CurrentWidth;

	/** Current branching depth (increases with each '[') */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|State")
	int32 Depth;

	/** Default constructor - starts at origin pointing up */
	FTurtleState()
		: Position(FVector::ZeroVector)
		, Forward(FVector::UpVector)      // Z-up
		, Left(FVector::LeftVector)       // -Y
		, Up(FVector::ForwardVector)      // X (perpendicular to Z-up)
		, CurrentWidth(5.0f)
		, Depth(0)
	{
	}

	/** Constructor with custom initial state */
	FTurtleState(const FVector& InPosition, const FVector& InForward, float InWidth)
		: Position(InPosition)
		, Forward(InForward.GetSafeNormal())
		, CurrentWidth(InWidth)
		, Depth(0)
	{
		// Calculate orthonormal basis
		if (FMath::Abs(Forward.Z) < 0.99f)
		{
			Left = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
		}
		else
		{
			Left = FVector::CrossProduct(FVector::ForwardVector, Forward).GetSafeNormal();
		}
		Up = FVector::CrossProduct(Forward, Left).GetSafeNormal();
	}
};

// ============================================================================
// FBranchSegment - Single Branch Segment Output
// ============================================================================

/**
 * Represents a single branch segment generated by the turtle interpreter.
 * Used as intermediate data between turtle interpretation and mesh generation.
 */
USTRUCT(BlueprintType)
struct LSYSTEMTREES_API FBranchSegment
{
	GENERATED_BODY()

	/** Start position of this branch segment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Branch")
	FVector StartPosition;

	/** End position of this branch segment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Branch")
	FVector EndPosition;

	/** Radius at start of segment (for tapering) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Branch")
	float StartRadius;

	/** Radius at end of segment (for tapering) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Branch")
	float EndRadius;

	/** Direction of the branch (normalized) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Branch")
	FVector Direction;

	/** Branching depth (0 = trunk, higher = smaller branches) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Branch")
	int32 Depth;

	/** Material/color index for variation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Branch")
	int32 MaterialIndex;

	/** Default constructor */
	FBranchSegment()
		: StartPosition(FVector::ZeroVector)
		, EndPosition(FVector::ZeroVector)
		, StartRadius(5.0f)
		, EndRadius(5.0f)
		, Direction(FVector::UpVector)
		, Depth(0)
		, MaterialIndex(0)
	{
	}

	/** Constructor with positions and radii */
	FBranchSegment(const FVector& InStart, const FVector& InEnd, float InStartRadius, float InEndRadius, int32 InDepth = 0)
		: StartPosition(InStart)
		, EndPosition(InEnd)
		, StartRadius(InStartRadius)
		, EndRadius(InEndRadius)
		, Depth(InDepth)
		, MaterialIndex(0)
	{
		FVector Delta = InEnd - InStart;
		Direction = Delta.GetSafeNormal();
	}

	/** Get the length of this segment */
	float GetLength() const
	{
		return FVector::Dist(StartPosition, EndPosition);
	}
};

// ============================================================================
// FLeafData - Leaf Placement Information
// ============================================================================

/**
 * Represents a single leaf to be placed on the tree.
 * Generated by TurtleInterpreter when 'L' symbol is encountered.
 */
USTRUCT(BlueprintType)
struct LSYSTEMTREES_API FLeafData
{
	GENERATED_BODY()

	/** Center position of the leaf */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leaf")
	FVector Position;

	/** Normal direction the leaf faces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leaf")
	FVector Normal;

	/** Up direction for leaf orientation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leaf")
	FVector UpDirection;

	/** Size of the leaf (Width, Height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leaf")
	FVector2D Size;

	/** Random rotation around the normal in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leaf")
	float Rotation;

	/** Branch depth where leaf was placed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leaf")
	int32 Depth;

	/** Default constructor */
	FLeafData()
		: Position(FVector::ZeroVector)
		, Normal(FVector::ForwardVector)
		, UpDirection(FVector::UpVector)
		, Size(10.0f, 15.0f)
		, Rotation(0.0f)
		, Depth(0)
	{
	}

	/** Constructor with position and orientation */
	FLeafData(const FVector& InPosition, const FVector& InNormal, const FVector& InUp, const FVector2D& InSize, int32 InDepth = 0)
		: Position(InPosition)
		, Normal(InNormal.GetSafeNormal())
		, UpDirection(InUp.GetSafeNormal())
		, Size(InSize)
		, Rotation(0.0f)
		, Depth(InDepth)
	{
	}
};

// ============================================================================
// FTurtleConfig - Turtle Interpretation Configuration
// ============================================================================

/**
 * Configuration settings for turtle interpretation of L-System strings.
 */
USTRUCT(BlueprintType)
struct LSYSTEMTREES_API FTurtleConfig
{
	GENERATED_BODY()

	/** Default rotation angle for +/- symbols (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Angles",
		meta = (ClampMin = "0", ClampMax = "180", UIMin = "0", UIMax = "90"))
	float DefaultAngle;

	/** Pitch angle for ^/& symbols (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Angles",
		meta = (ClampMin = "0", ClampMax = "180", UIMin = "0", UIMax = "90"))
	float PitchAngle;

	/** Roll angle for \/\ symbols (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Angles",
		meta = (ClampMin = "0", ClampMax = "180", UIMin = "0", UIMax = "90"))
	float RollAngle;

	/** Distance to move forward for F symbol */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Movement",
		meta = (ClampMin = "0.1", UIMin = "1", UIMax = "100"))
	float StepLength;

	/** Initial branch/trunk width */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Width",
		meta = (ClampMin = "0.1", UIMin = "1", UIMax = "50"))
	float InitialWidth;

	/** Width multiplier when entering a branch (per depth level) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Width",
		meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.5", UIMax = "0.9"))
	float WidthFalloff;

	/** Minimum width before branches stop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Width",
		meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "5"))
	float MinWidth;

	/** Tropism strength - how much branches bend toward gravity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Tropism",
		meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "0.5"))
	float TropismStrength;

	/** Direction of tropism (typically gravity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Tropism")
	FVector GravityVector;

	/** Initial position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Initial")
	FVector InitialPosition;

	/** Initial forward direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Initial")
	FVector InitialForward;

	/** Default leaf size */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Leaves")
	FVector2D LeafSize;

	/** Random seed for reproducible results */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turtle|Random")
	int32 RandomSeed;

	/** Default constructor */
	FTurtleConfig()
		: DefaultAngle(25.0f)
		, PitchAngle(25.0f)
		, RollAngle(25.0f)
		, StepLength(10.0f)
		, InitialWidth(5.0f)
		, WidthFalloff(0.7f)
		, MinWidth(0.5f)
		, TropismStrength(0.1f)
		, GravityVector(0.0f, 0.0f, -1.0f)
		, InitialPosition(FVector::ZeroVector)
		, InitialForward(FVector::UpVector)
		, LeafSize(10.0f, 15.0f)
		, RandomSeed(0)
	{
	}
};

// ============================================================================
// FTreeLODLevel - LOD Configuration
// ============================================================================

/**
 * Configuration for a single LOD level of tree mesh.
 */
USTRUCT(BlueprintType)
struct LSYSTEMTREES_API FTreeLODLevel
{
	GENERATED_BODY()

	/** Number of radial segments for branch cylinders */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD",
		meta = (ClampMin = "3", ClampMax = "32", UIMin = "4", UIMax = "16"))
	int32 RadialSegments;

	/** Screen size threshold for this LOD (0-1, higher = switch at larger screen size) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD",
		meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float ScreenSize;

	/** Whether to include leaves at this LOD level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
	bool bIncludeLeaves;

	/** Default constructor */
	FTreeLODLevel()
		: RadialSegments(8)
		, ScreenSize(0.5f)
		, bIncludeLeaves(true)
	{
	}

	/** Constructor with parameters */
	FTreeLODLevel(int32 InSegments, float InScreenSize, bool InIncludeLeaves = true)
		: RadialSegments(InSegments)
		, ScreenSize(InScreenSize)
		, bIncludeLeaves(InIncludeLeaves)
	{
	}
};

// ============================================================================
// FTreeMeshData - Generated Mesh Data
// ============================================================================

/**
 * Contains all mesh data generated for a tree at a single LOD level.
 * This data can be passed directly to UProceduralMeshComponent.
 */
USTRUCT(BlueprintType)
struct LSYSTEMTREES_API FTreeMeshData
{
	GENERATED_BODY()

	/** Vertex positions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TArray<FVector> Vertices;

	/** Triangle indices (3 per triangle) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TArray<int32> Triangles;

	/** Vertex normals */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TArray<FVector> Normals;

	/** UV coordinates */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TArray<FVector2D> UVs;

	/** Vertex colors */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TArray<FLinearColor> VertexColors;

	/** Tangent vectors for normal mapping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TArray<FVector> Tangents;

	/** Number of vertices in branch section (rest are leaves) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	int32 BranchVertexCount;

	/** Number of triangles in branch section */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	int32 BranchTriangleCount;

	/** Default constructor */
	FTreeMeshData()
		: BranchVertexCount(0)
		, BranchTriangleCount(0)
	{
	}

	/** Reset all mesh data */
	void Reset()
	{
		Vertices.Empty();
		Triangles.Empty();
		Normals.Empty();
		UVs.Empty();
		VertexColors.Empty();
		Tangents.Empty();
		BranchVertexCount = 0;
		BranchTriangleCount = 0;
	}

	/** Check if mesh data is valid */
	bool IsValid() const
	{
		return Vertices.Num() > 0 && Triangles.Num() > 0;
	}

	/** Get total vertex count */
	int32 GetVertexCount() const
	{
		return Vertices.Num();
	}

	/** Get total triangle count */
	int32 GetTriangleCount() const
	{
		return Triangles.Num() / 3;
	}
};

// ============================================================================
// FTreeGeometryConfig - Geometry Generation Configuration
// ============================================================================

/**
 * Configuration for tree mesh geometry generation.
 */
USTRUCT(BlueprintType)
struct LSYSTEMTREES_API FTreeGeometryConfig
{
	GENERATED_BODY()

	/** LOD levels to generate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|LOD")
	TArray<FTreeLODLevel> LODLevels;

	/** Default leaf size (width, height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Leaves")
	FVector2D LeafSize;

	/** Maximum random rotation for leaves (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Leaves",
		meta = (ClampMin = "0", ClampMax = "180"))
	float LeafRandomRotation;

	/** Whether to generate collision for LOD0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Collision")
	bool bGenerateCollision;

	/** UV tiling for bark texture (along branch length) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|UVs",
		meta = (ClampMin = "0.1", UIMin = "0.5", UIMax = "5"))
	float BarkUVTiling;

	/** Default constructor with default LOD levels */
	FTreeGeometryConfig()
		: LeafSize(10.0f, 15.0f)
		, LeafRandomRotation(30.0f)
		, bGenerateCollision(true)
		, BarkUVTiling(1.0f)
	{
		// Default 3 LOD levels
		LODLevels.Add(FTreeLODLevel(16, 1.0f, true));   // LOD0: High detail
		LODLevels.Add(FTreeLODLevel(8, 0.5f, true));    // LOD1: Medium detail
		LODLevels.Add(FTreeLODLevel(4, 0.25f, false));  // LOD2: Low detail, no leaves
	}
};

// ============================================================================
// DELEGATES (must be declared AFTER structs they reference)
// ============================================================================

/** Delegate fired when async generation completes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSystemGenerationComplete, const FLSystemGenerationResult&, Result);

/** Delegate fired after each iteration completes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSystemIterationComplete, int32, Iteration, const FString&, CurrentString);

/** Delegate fired to report generation progress (0.0 - 1.0) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSystemGenerationProgress, float, Progress);
