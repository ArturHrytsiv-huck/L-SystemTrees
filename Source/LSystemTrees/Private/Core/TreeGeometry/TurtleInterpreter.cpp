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

	// Move forward
	CurrentState.Position += CurrentState.Forward * ActiveConfig.StepLength;

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

		OutputSegments.Add(Segment);
	}

	// Update width for next segment
	CurrentState.CurrentWidth = EndWidth;
}

void UTurtleInterpreter::HandleRotateYaw(float AngleDegrees)
{
	// Rotate Forward and Left around Up axis
	CurrentState.Forward = RotateVector(CurrentState.Forward, CurrentState.Up, AngleDegrees);
	CurrentState.Left = RotateVector(CurrentState.Left, CurrentState.Up, AngleDegrees);

	// Ensure orthonormality
	ReorthogonalizeBasis();
}

void UTurtleInterpreter::HandleRotatePitch(float AngleDegrees)
{
	// Rotate Forward and Up around Left axis
	CurrentState.Forward = RotateVector(CurrentState.Forward, CurrentState.Left, AngleDegrees);
	CurrentState.Up = RotateVector(CurrentState.Up, CurrentState.Left, AngleDegrees);

	// Ensure orthonormality
	ReorthogonalizeBasis();
}

void UTurtleInterpreter::HandleRotateRoll(float AngleDegrees)
{
	// Rotate Left and Up around Forward axis
	CurrentState.Left = RotateVector(CurrentState.Left, CurrentState.Forward, AngleDegrees);
	CurrentState.Up = RotateVector(CurrentState.Up, CurrentState.Forward, AngleDegrees);

	// Ensure orthonormality
	ReorthogonalizeBasis();
}

void UTurtleInterpreter::HandlePushState()
{
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
}

void UTurtleInterpreter::ApplyTropism()
{
	if (ActiveConfig.TropismStrength <= 0.0f)
	{
		return;
	}

	// Blend forward direction toward gravity
	const FVector TropismDir = ActiveConfig.GravityVector.GetSafeNormal();
	const float Strength = ActiveConfig.TropismStrength;

	// Calculate rotation axis (perpendicular to both forward and gravity)
	FVector RotationAxis = FVector::CrossProduct(CurrentState.Forward, TropismDir);

	if (!RotationAxis.IsNearlyZero())
	{
		RotationAxis.Normalize();

		// Calculate rotation angle based on strength
		const float MaxAngle = Strength * 5.0f; // Small angle per step
		const float Dot = FVector::DotProduct(CurrentState.Forward, TropismDir);
		const float AngleToGravity = FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f));

		// Rotate toward gravity
		const float RotationAngle = FMath::Min(MaxAngle, FMath::RadiansToDegrees(AngleToGravity) * Strength);

		if (RotationAngle > 0.01f)
		{
			CurrentState.Forward = RotateVector(CurrentState.Forward, RotationAxis, RotationAngle);
			CurrentState.Up = RotateVector(CurrentState.Up, RotationAxis, RotationAngle);
			CurrentState.Left = RotateVector(CurrentState.Left, RotationAxis, RotationAngle);
			ReorthogonalizeBasis();
		}
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
