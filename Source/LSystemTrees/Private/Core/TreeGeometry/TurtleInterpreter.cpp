// TurtleInterpreter.cpp
// Interprets L-System strings as 3D turtle graphics commands
// Part of LSystemTrees Plugin - Phase 3

#include "Core/TreeGeometry/TurtleInterpreter.h"
#include "Core/Utilities/TreeMath.h"

DEFINE_LOG_CATEGORY(LogTurtle);

// ============================================================================
// Constructor
// ============================================================================

UTurtleInterpreter::UTurtleInterpreter()
	: MaxDepthReached(0)
	, SymbolsProcessed(0)
{
	RandomStream.Initialize(FMath::Rand());
}

// ============================================================================
// Main Interpretation
// ============================================================================

void UTurtleInterpreter::InterpretString(const FString& LSystemString,
                                          const FTurtleConfig& Config,
                                          TArray<FBranchSegment>& OutSegments,
                                          TArray<FLeafData>& OutLeaves)
{
	// Initialize
	Reset();
	ActiveConfig = Config;
	InitializeState(Config);

	// Initialize random stream
	if (Config.RandomSeed != 0)
	{
		RandomStream.Initialize(Config.RandomSeed);
	}
	else
	{
		RandomStream.Initialize(FMath::Rand());
	}

	UE_LOG(LogTurtle, Verbose, TEXT("Interpreting L-System string of length %d"), LSystemString.Len());

	// Process each symbol
	for (int32 i = 0; i < LSystemString.Len(); ++i)
	{
		ProcessSymbol(LSystemString[i]);
		SymbolsProcessed++;
	}

	// Copy output
	OutSegments = OutputSegments;
	OutLeaves = OutputLeaves;

	UE_LOG(LogTurtle, Log, TEXT("Interpretation complete: %d segments, %d leaves, max depth %d"),
	       OutputSegments.Num(), OutputLeaves.Num(), MaxDepthReached);
}

TArray<FBranchSegment> UTurtleInterpreter::InterpretToSegments(const FString& LSystemString,
                                                                const FTurtleConfig& Config)
{
	TArray<FBranchSegment> Segments;
	TArray<FLeafData> Leaves;
	InterpretString(LSystemString, Config, Segments, Leaves);
	return Segments;
}

// ============================================================================
// Symbol Handlers
// ============================================================================

void UTurtleInterpreter::HandleForward(bool bDraw)
{
	// Store start position
	const FVector StartPosition = CurrentState.Position;
	const float StartWidth = CurrentState.CurrentWidth;

	// Calculate end width (taper along segment)
	const float EndWidth = FMath::Max(StartWidth * 0.95f, ActiveConfig.MinWidth);

	// Calculate step length with random variation
	float ActualStepLength = ActiveConfig.StepLength;
	if (ActiveConfig.StepLengthVariation > 0.0f)
	{
		const float Variation = RandomStream.FRandRange(-ActiveConfig.StepLengthVariation, ActiveConfig.StepLengthVariation);
		ActualStepLength *= (1.0f + Variation);
		ActualStepLength = FMath::Max(ActualStepLength, 0.1f); // Ensure positive
	}

	// Move forward
	CurrentState.Position += CurrentState.Forward * ActualStepLength;

	// Apply tropism after movement
	if (ActiveConfig.TropismStrength > 0.0f)
	{
		ApplyTropism();
	}

	// Create segment if drawing
	if (bDraw && StartWidth >= ActiveConfig.MinWidth)
	{
		FBranchSegment Segment;
		Segment.StartPosition = StartPosition;
		Segment.EndPosition = CurrentState.Position;
		Segment.StartRadius = StartWidth;
		Segment.EndRadius = EndWidth;
		Segment.Direction = CurrentState.Forward;
		Segment.Depth = CurrentState.Depth;
		Segment.MaterialIndex = 0;
		Segment.ParentSegmentIndex = CurrentState.LastSegmentIndex;

		// Update last segment index before adding
		CurrentState.LastSegmentIndex = OutputSegments.Num();
		OutputSegments.Add(Segment);
	}

	// Update width for next segment
	CurrentState.CurrentWidth = EndWidth;
}

float UTurtleInterpreter::GetRandomAngleVariation()
{
	if (ActiveConfig.AngleVariationMin >= ActiveConfig.AngleVariationMax)
	{
		return 0.0f;
	}
	return RandomStream.FRandRange(ActiveConfig.AngleVariationMin, ActiveConfig.AngleVariationMax);
}

float UTurtleInterpreter::GetRandomPitchVariation()
{
	if (ActiveConfig.PitchVariationMin >= ActiveConfig.PitchVariationMax)
	{
		return 0.0f;
	}
	return RandomStream.FRandRange(ActiveConfig.PitchVariationMin, ActiveConfig.PitchVariationMax);
}

bool UTurtleInterpreter::ShouldFlipPitch()
{
	if (!ActiveConfig.bRandomizePitchDirection)
	{
		return false;
	}
	return RandomStream.FRand() < ActiveConfig.PitchFlipProbability;
}

void UTurtleInterpreter::HandleRotateYaw(float AngleDegrees)
{
	// Add random variation to angle
	const float FinalAngle = AngleDegrees + GetRandomAngleVariation();

	// Rotate Forward and Left around Up axis
	CurrentState.Forward = RotateVector(CurrentState.Forward, CurrentState.Up, FinalAngle);
	CurrentState.Left = RotateVector(CurrentState.Left, CurrentState.Up, FinalAngle);

	// Ensure orthonormality
	ReorthogonalizeBasis();
}

void UTurtleInterpreter::HandleRotatePitch(float AngleDegrees)
{
	// Optionally flip pitch direction for more organic variation
	float EffectiveAngle = AngleDegrees;
	if (ShouldFlipPitch())
	{
		EffectiveAngle = -AngleDegrees;
	}

	// Add pitch-specific random variation
	const float FinalAngle = EffectiveAngle + GetRandomPitchVariation();

	// Rotate Forward and Up around Left axis
	CurrentState.Forward = RotateVector(CurrentState.Forward, CurrentState.Left, FinalAngle);
	CurrentState.Up = RotateVector(CurrentState.Up, CurrentState.Left, FinalAngle);

	// Ensure orthonormality
	ReorthogonalizeBasis();
}

void UTurtleInterpreter::HandleRotateRoll(float AngleDegrees)
{
	// Add random variation to angle
	const float FinalAngle = AngleDegrees + GetRandomAngleVariation();

	// Rotate Left and Up around Forward axis
	CurrentState.Left = RotateVector(CurrentState.Left, CurrentState.Forward, FinalAngle);
	CurrentState.Up = RotateVector(CurrentState.Up, CurrentState.Forward, FinalAngle);

	// Ensure orthonormality
	ReorthogonalizeBasis();
}

void UTurtleInterpreter::HandlePushState()
{
	// Check if we're already skipping a branch
	if (SkipBranchDepth > 0)
	{
		SkipBranchDepth++;
		return;
	}

	// Check branch probability - should we skip this branch?
	if (ActiveConfig.BranchProbability < 1.0f)
	{
		const float Roll = RandomStream.FRand();
		if (Roll > ActiveConfig.BranchProbability)
		{
			// Skip this branch
			SkipBranchDepth = 1;
			UE_LOG(LogTurtle, Verbose, TEXT("Skipping branch (rolled %.2f, probability %.2f)"),
			       Roll, ActiveConfig.BranchProbability);
			return;
		}
	}

	// Save current state
	StateStack.Push(CurrentState);

	// Increase depth
	CurrentState.Depth++;

	// Apply width falloff for new branch
	CurrentState.CurrentWidth *= ActiveConfig.WidthFalloff;
	CurrentState.CurrentWidth = FMath::Max(CurrentState.CurrentWidth, ActiveConfig.MinWidth);

	// Track maximum depth
	MaxDepthReached = FMath::Max(MaxDepthReached, CurrentState.Depth);

	UE_LOG(LogTurtle, Verbose, TEXT("Push state: depth now %d, width %.2f"),
	       CurrentState.Depth, CurrentState.CurrentWidth);
}

void UTurtleInterpreter::HandlePopState()
{
	// Check if we're exiting a skipped branch
	if (SkipBranchDepth > 0)
	{
		SkipBranchDepth--;
		return;
	}

	if (StateStack.Num() > 0)
	{
		CurrentState = StateStack.Pop();

		UE_LOG(LogTurtle, Verbose, TEXT("Pop state: depth now %d, position (%.1f, %.1f, %.1f)"),
		       CurrentState.Depth, CurrentState.Position.X, CurrentState.Position.Y, CurrentState.Position.Z);
	}
	else
	{
		UE_LOG(LogTurtle, Warning, TEXT("Attempted to pop empty state stack"));
	}
}

void UTurtleInterpreter::HandleTurnAround()
{
	// Rotate 180 degrees around Up axis
	HandleRotateYaw(180.0f);
}

void UTurtleInterpreter::HandlePlaceLeaf()
{
	// Create leaf at current position
	FLeafData Leaf;
	Leaf.Position = CurrentState.Position;
	Leaf.Normal = CurrentState.Forward;  // Leaf faces forward direction
	Leaf.UpDirection = CurrentState.Up;
	Leaf.Size = ActiveConfig.LeafSize;
	Leaf.Depth = CurrentState.Depth;

	// Add random rotation
	Leaf.Rotation = RandomStream.FRandRange(-30.0f, 30.0f);

	OutputLeaves.Add(Leaf);

	UE_LOG(LogTurtle, Verbose, TEXT("Placed leaf at (%.1f, %.1f, %.1f)"),
	       Leaf.Position.X, Leaf.Position.Y, Leaf.Position.Z);
}

// ============================================================================
// Internal Methods
// ============================================================================

void UTurtleInterpreter::InitializeState(const FTurtleConfig& Config)
{
	CurrentState.Position = Config.InitialPosition;
	CurrentState.Forward = Config.InitialForward.GetSafeNormal();
	CurrentState.CurrentWidth = Config.InitialWidth;
	CurrentState.Depth = 0;

	// Calculate initial orthonormal basis
	if (FMath::Abs(CurrentState.Forward.Z) < 0.99f)
	{
		CurrentState.Left = FVector::CrossProduct(FVector::UpVector, CurrentState.Forward).GetSafeNormal();
	}
	else
	{
		CurrentState.Left = FVector::CrossProduct(FVector::ForwardVector, CurrentState.Forward).GetSafeNormal();
	}
	CurrentState.Up = FVector::CrossProduct(CurrentState.Forward, CurrentState.Left).GetSafeNormal();

	// Apply initial random roll for variety between trees
	if (Config.InitialRandomRoll > 0.0f)
	{
		const float RandomRoll = RandomStream.FRandRange(0.0f, Config.InitialRandomRoll);
		CurrentState.Left = RotateVector(CurrentState.Left, CurrentState.Forward, RandomRoll);
		CurrentState.Up = RotateVector(CurrentState.Up, CurrentState.Forward, RandomRoll);
		ReorthogonalizeBasis();

		UE_LOG(LogTurtle, Verbose, TEXT("Applied initial random roll: %.1f degrees"), RandomRoll);
	}

	UE_LOG(LogTurtle, Verbose, TEXT("Initialized turtle at (%.1f, %.1f, %.1f) facing (%.2f, %.2f, %.2f)"),
	       CurrentState.Position.X, CurrentState.Position.Y, CurrentState.Position.Z,
	       CurrentState.Forward.X, CurrentState.Forward.Y, CurrentState.Forward.Z);
}

void UTurtleInterpreter::Reset()
{
	CurrentState = FTurtleState();
	StateStack.Empty();
	OutputSegments.Empty();
	OutputLeaves.Empty();
	MaxDepthReached = 0;
	SymbolsProcessed = 0;
	SkipBranchDepth = 0;
}

void UTurtleInterpreter::ApplyTropism()
{
	if (ActiveConfig.TropismStrength <= 0.0f)
	{
		return;
	}

	// Get horizontal component of Forward direction
	FVector2D Horizontal(CurrentState.Forward.X, CurrentState.Forward.Y);
	const float HorizontalMag = Horizontal.Size();

	// Skip tropism for nearly vertical branches (no horizontal direction to preserve)
	if (HorizontalMag < KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Calculate rotation axis perpendicular to horizontal direction IN the XY plane
	// This ensures the horizontal direction stays unchanged while tilting down
	// Axis is 90° rotated from horizontal direction in XY plane
	FVector RotationAxis(-Horizontal.Y / HorizontalMag, Horizontal.X / HorizontalMag, 0.0f);

	// Calculate current elevation angle (angle from horizontal plane)
	const float CurrentElevation = FMath::Atan2(CurrentState.Forward.Z, HorizontalMag);

	// Calculate rotation amount - tilt downward
	const float MaxRotationDeg = ActiveConfig.TropismStrength * 5.0f;
	const float MaxRotationRad = FMath::DegreesToRadians(MaxRotationDeg);

	// Target elevation (don't go past -80 degrees / nearly straight down)
	const float MinElevation = FMath::DegreesToRadians(-80.0f);
	const float TargetElevation = FMath::Max(CurrentElevation - MaxRotationRad, MinElevation);

	// Only rotate if we have room to tilt down
	const float RotationAngle = FMath::RadiansToDegrees(CurrentElevation - TargetElevation);

	if (RotationAngle > 0.01f)
	{
		// Positive rotation around our axis tilts downward while preserving horizontal direction
		CurrentState.Forward = RotateVector(CurrentState.Forward, RotationAxis, RotationAngle);
		CurrentState.Up = RotateVector(CurrentState.Up, RotationAxis, RotationAngle);
		CurrentState.Left = RotateVector(CurrentState.Left, RotationAxis, RotationAngle);
		ReorthogonalizeBasis();
	}
}

void UTurtleInterpreter::ReorthogonalizeBasis()
{
	// Normalize forward
	CurrentState.Forward = CurrentState.Forward.GetSafeNormal();

	// Make Left perpendicular to Forward using Gram-Schmidt
	CurrentState.Left = CurrentState.Left -
		CurrentState.Forward * FVector::DotProduct(CurrentState.Left, CurrentState.Forward);
	CurrentState.Left = CurrentState.Left.GetSafeNormal();

	// Handle degenerate case
	if (CurrentState.Left.IsNearlyZero())
	{
		if (FMath::Abs(CurrentState.Forward.Z) < 0.9f)
		{
			CurrentState.Left = FVector::CrossProduct(FVector::UpVector, CurrentState.Forward).GetSafeNormal();
		}
		else
		{
			CurrentState.Left = FVector::CrossProduct(FVector::ForwardVector, CurrentState.Forward).GetSafeNormal();
		}
	}

	// Up is cross product
	CurrentState.Up = FVector::CrossProduct(CurrentState.Forward, CurrentState.Left).GetSafeNormal();
}

FVector UTurtleInterpreter::RotateVector(const FVector& Vector, const FVector& Axis, float AngleDegrees)
{
	// Rodrigues' rotation formula
	if (Axis.IsNearlyZero())
	{
		return Vector;
	}

	const FVector K = Axis.GetSafeNormal();
	const float AngleRad = FMath::DegreesToRadians(AngleDegrees);
	const float CosAngle = FMath::Cos(AngleRad);
	const float SinAngle = FMath::Sin(AngleRad);

	return Vector * CosAngle +
	       FVector::CrossProduct(K, Vector) * SinAngle +
	       K * FVector::DotProduct(K, Vector) * (1.0f - CosAngle);
}

void UTurtleInterpreter::ProcessSymbol(TCHAR Symbol)
{
	// When skipping a branch, only process [ and ] to track depth
	if (SkipBranchDepth > 0)
	{
		if (Symbol == TEXT('['))
		{
			SkipBranchDepth++;
		}
		else if (Symbol == TEXT(']'))
		{
			SkipBranchDepth--;
		}
		// Skip all other symbols
		return;
	}

	switch (Symbol)
	{
	// Movement
	case TEXT('F'):
		HandleForward(true);
		break;
	case TEXT('f'):
		HandleForward(false);
		break;

	// Yaw (rotation around Up)
	case TEXT('+'):
		HandleRotateYaw(ActiveConfig.DefaultAngle);
		break;
	case TEXT('-'):
		HandleRotateYaw(-ActiveConfig.DefaultAngle);
		break;

	// Pitch (rotation around Left)
	case TEXT('^'):
		HandleRotatePitch(ActiveConfig.PitchAngle);
		break;
	case TEXT('&'):
		HandleRotatePitch(-ActiveConfig.PitchAngle);
		break;

	// Roll (rotation around Forward)
	case TEXT('\\'):
		HandleRotateRoll(ActiveConfig.RollAngle);
		break;
	case TEXT('/'):
		HandleRotateRoll(-ActiveConfig.RollAngle);
		break;

	// Turn around
	case TEXT('|'):
		HandleTurnAround();
		break;

	// Branching
	case TEXT('['):
		HandlePushState();
		break;
	case TEXT(']'):
		HandlePopState();
		break;

	// Leaves
	case TEXT('L'):
		HandlePlaceLeaf();
		break;

	// Ignored symbols (placeholders in L-System rules)
	case TEXT('X'):
	case TEXT('Y'):
	case TEXT('Z'):
	case TEXT('A'):
	case TEXT('B'):
	case TEXT('G'):
		// These are often used as variables in L-System rules
		// and don't produce any turtle action
		break;

	default:
		// Unknown symbol - ignore silently
		// Many L-System strings contain symbols that don't map to turtle actions
		break;
	}
}
