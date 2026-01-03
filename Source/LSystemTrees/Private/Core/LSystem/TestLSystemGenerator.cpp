// TestLSystemGenerator.cpp
// Test actor for validating L-System generator functionality
// Part of LSystemTrees Plugin - Phase 2 & 3

#include "Core/LSystem/TestLSystemGenerator.h"
#include "Core/LSystem/LSystemRule.h"
#include "Core/TreeGeometry/TurtleInterpreter.h"
#include "Core/TreeGeometry/TreeGeometry.h"
#include "Core/Utilities/TreeMath.h"

ATestLSystemGenerator::ATestLSystemGenerator()
	: bVerboseLogging(true)
	, PerformanceIterations(8)
	, bRunTestsOnBeginPlay(false)
	, PassedTests(0)
	, FailedTests(0)
	, TotalTestTimeMs(0.0f)
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATestLSystemGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (bRunTestsOnBeginPlay)
	{
		RunAllTests();
	}
}

// ============================================================================
// Main Test Runner
// ============================================================================

bool ATestLSystemGenerator::RunAllTests()
{
	ResetTestCounters();

	const double StartTime = FPlatformTime::Seconds();

	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("========================================"));
	UE_LOG(LogLSystem, Log, TEXT("  L-SYSTEM GENERATOR TESTS"));
	UE_LOG(LogLSystem, Log, TEXT("========================================"));

	bool bAllPassed = true;

	bAllPassed &= TestSimpleGeneration();
	bAllPassed &= TestStochasticGeneration();
	bAllPassed &= TestContextSensitive();
	bAllPassed &= TestEdgeCases();
	bAllPassed &= TestKnownPatterns();
	bAllPassed &= TestPerformance();
	// Note: TestAsyncGeneration() requires async wait, skip in synchronous run

	// Phase 3 Tests
	bAllPassed &= TestTreeMath();
	bAllPassed &= TestTurtleInterpreter();
	bAllPassed &= TestTreeGeometry();
	bAllPassed &= TestFullPipeline();

	const double EndTime = FPlatformTime::Seconds();
	TotalTestTimeMs = static_cast<float>((EndTime - StartTime) * 1000.0);

	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("========================================"));
	UE_LOG(LogLSystem, Log, TEXT("  TEST RESULTS"));
	UE_LOG(LogLSystem, Log, TEXT("========================================"));
	UE_LOG(LogLSystem, Log, TEXT("  Passed: %d"), PassedTests);
	UE_LOG(LogLSystem, Log, TEXT("  Failed: %d"), FailedTests);
	UE_LOG(LogLSystem, Log, TEXT("  Total:  %d"), PassedTests + FailedTests);
	UE_LOG(LogLSystem, Log, TEXT("  Time:   %.2f ms"), TotalTestTimeMs);
	UE_LOG(LogLSystem, Log, TEXT("  Status: %s"), bAllPassed ? TEXT("ALL PASSED") : TEXT("SOME FAILED"));
	UE_LOG(LogLSystem, Log, TEXT("========================================"));
	UE_LOG(LogLSystem, Log, TEXT(""));

	return bAllPassed;
}

// ============================================================================
// Simple Generation Tests
// ============================================================================

bool ATestLSystemGenerator::TestSimpleGeneration()
{
	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("--- TestSimpleGeneration ---"));

	int32 InitialFailed = FailedTests;

	// Test 1: Basic string replacement (Algae pattern)
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("A"));
		Gen->AddRuleSimple(TEXT("A"), TEXT("AB"));
		Gen->AddRuleSimple(TEXT("B"), TEXT("A"));

		FString Result = Gen->GenerateString(3);

		// A -> AB -> ABA -> ABAAB
		VerifyOutput(Result, TEXT("ABAAB"), TEXT("BasicReplacement"));
	}

	// Test 2: Multiple characters in axiom
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("AB"));
		Gen->AddRuleSimple(TEXT("A"), TEXT("AA"));

		FString Result = Gen->GenerateString(2);

		// AB -> AAB -> AAAAB
		VerifyOutput(Result, TEXT("AAAAB"), TEXT("MultiCharAxiom"));
	}

	// Test 3: No matching rules (identity)
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("XYZ"));
		Gen->AddRuleSimple(TEXT("A"), TEXT("B"));

		FString Result = Gen->GenerateString(1);

		// No rules match X, Y, Z - string unchanged
		VerifyOutput(Result, TEXT("XYZ"), TEXT("IdentityRule"));
	}

	// Test 4: Single character expansion
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("F"));
		Gen->AddRuleSimple(TEXT("F"), TEXT("FF"));

		FString Result = Gen->GenerateString(3);

		// F -> FF -> FFFF -> FFFFFFFF (2^3 = 8 F's)
		VerifyOutput(Result, TEXT("FFFFFFFF"), TEXT("ExponentialGrowth"));
	}

	// Test 5: Multiple rules for different symbols
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("AB"));
		Gen->AddRuleSimple(TEXT("A"), TEXT("X"));
		Gen->AddRuleSimple(TEXT("B"), TEXT("Y"));

		FString Result = Gen->GenerateString(1);

		VerifyOutput(Result, TEXT("XY"), TEXT("MultipleSymbolRules"));
	}

	return FailedTests == InitialFailed;
}

// ============================================================================
// Stochastic Generation Tests
// ============================================================================

bool ATestLSystemGenerator::TestStochasticGeneration()
{
	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("--- TestStochasticGeneration ---"));

	int32 InitialFailed = FailedTests;

	// Test 1: Reproducibility with same seed
	{
		ULSystemGenerator* Gen1 = CreateTestGenerator();
		Gen1->Initialize(TEXT("F"));
		Gen1->AddRuleStochastic(TEXT("F"), TEXT("F[+F]"), 0.5f);
		Gen1->AddRuleStochastic(TEXT("F"), TEXT("F[-F]"), 0.5f);
		Gen1->SetRandomSeed(12345);

		FString Result1 = Gen1->GenerateString(5);

		ULSystemGenerator* Gen2 = CreateTestGenerator();
		Gen2->Initialize(TEXT("F"));
		Gen2->AddRuleStochastic(TEXT("F"), TEXT("F[+F]"), 0.5f);
		Gen2->AddRuleStochastic(TEXT("F"), TEXT("F[-F]"), 0.5f);
		Gen2->SetRandomSeed(12345);

		FString Result2 = Gen2->GenerateString(5);

		bool bPassed = (Result1 == Result2);
		LogTestResult(TEXT("SeedReproducibility"), bPassed,
		              bPassed ? TEXT("") : TEXT("Results differ with same seed"));
	}

	// Test 2: Different seeds produce different results (probabilistically)
	{
		ULSystemGenerator* Gen1 = CreateTestGenerator();
		Gen1->Initialize(TEXT("F"));
		Gen1->AddRuleStochastic(TEXT("F"), TEXT("FF"), 0.5f);
		Gen1->AddRuleStochastic(TEXT("F"), TEXT("F"), 0.5f);
		Gen1->SetRandomSeed(11111);

		FString Result1 = Gen1->GenerateString(6);

		ULSystemGenerator* Gen2 = CreateTestGenerator();
		Gen2->Initialize(TEXT("F"));
		Gen2->AddRuleStochastic(TEXT("F"), TEXT("FF"), 0.5f);
		Gen2->AddRuleStochastic(TEXT("F"), TEXT("F"), 0.5f);
		Gen2->SetRandomSeed(99999);

		FString Result2 = Gen2->GenerateString(6);

		// Very unlikely to be equal with different seeds
		bool bPassed = (Result1 != Result2);
		LogTestResult(TEXT("DifferentSeeds"), bPassed,
		              bPassed ? TEXT("") : TEXT("Same result (unlikely)"));
	}

	// Test 3: Probability-based selection (statistical test)
	{
		// Run many times and check distribution
		int32 CountA = 0;
		int32 CountB = 0;
		const int32 NumTrials = 100;

		for (int32 i = 0; i < NumTrials; ++i)
		{
			ULSystemGenerator* Gen = CreateTestGenerator();
			Gen->Initialize(TEXT("X"));
			Gen->AddRuleStochastic(TEXT("X"), TEXT("A"), 0.5f);
			Gen->AddRuleStochastic(TEXT("X"), TEXT("B"), 0.5f);
			Gen->SetRandomSeed(i * 7919); // Different seed each time

			FString Result = Gen->GenerateString(1);
			if (Result == TEXT("A")) CountA++;
			else if (Result == TEXT("B")) CountB++;
		}

		// With 50/50 probability, expect roughly equal distribution (allow 20% tolerance)
		float Ratio = static_cast<float>(CountA) / static_cast<float>(NumTrials);
		bool bPassed = (Ratio > 0.3f && Ratio < 0.7f);
		LogTestResult(TEXT("ProbabilityDistribution"), bPassed,
		              FString::Printf(TEXT("A=%d, B=%d, Ratio=%.2f"), CountA, CountB, Ratio));
	}

	return FailedTests == InitialFailed;
}

// ============================================================================
// Context-Sensitive Tests
// ============================================================================

bool ATestLSystemGenerator::TestContextSensitive()
{
	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("--- TestContextSensitive ---"));

	int32 InitialFailed = FailedTests;

	// Test 1: Full context match (A<B>C -> X)
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("ABC"));
		Gen->AddContextRule(TEXT("A"), TEXT("B"), TEXT("C"), TEXT("X"));

		FString Result = Gen->GenerateString(1);

		// B is replaced with X only when preceded by A and followed by C
		VerifyOutput(Result, TEXT("AXC"), TEXT("FullContextMatch"));
	}

	// Test 2: Left context only (A<B -> X)
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("AB"));
		Gen->AddContextRule(TEXT("A"), TEXT("B"), TEXT(""), TEXT("X"));

		FString Result = Gen->GenerateString(1);

		VerifyOutput(Result, TEXT("AX"), TEXT("LeftContextOnly"));
	}

	// Test 3: Right context only (B>C -> X)
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("BC"));
		Gen->AddContextRule(TEXT(""), TEXT("B"), TEXT("C"), TEXT("X"));

		FString Result = Gen->GenerateString(1);

		VerifyOutput(Result, TEXT("XC"), TEXT("RightContextOnly"));
	}

	// Test 4: Context rule priority over simple rule
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("ABC"));
		Gen->AddRuleSimple(TEXT("B"), TEXT("Y"));  // General rule
		Gen->AddContextRule(TEXT("A"), TEXT("B"), TEXT("C"), TEXT("X"));  // Specific rule

		FString Result = Gen->GenerateString(1);

		// Context rule should win
		VerifyOutput(Result, TEXT("AXC"), TEXT("ContextPriority"));
	}

	// Test 5: Context doesn't match
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("XBY"));
		Gen->AddContextRule(TEXT("A"), TEXT("B"), TEXT("C"), TEXT("Z"));
		Gen->AddRuleSimple(TEXT("B"), TEXT("W"));

		FString Result = Gen->GenerateString(1);

		// Context A<B>C doesn't match XBY, so simple rule applies
		VerifyOutput(Result, TEXT("XWY"), TEXT("ContextNoMatch"));
	}

	// Test 6: Multiple context rules
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("ABCABD"));
		Gen->AddContextRule(TEXT("A"), TEXT("B"), TEXT("C"), TEXT("X"));
		Gen->AddContextRule(TEXT("A"), TEXT("B"), TEXT("D"), TEXT("Y"));

		FString Result = Gen->GenerateString(1);

		VerifyOutput(Result, TEXT("AXCAYD"), TEXT("MultipleContextRules"));
	}

	return FailedTests == InitialFailed;
}

// ============================================================================
// Edge Cases Tests
// ============================================================================

bool ATestLSystemGenerator::TestEdgeCases()
{
	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("--- TestEdgeCases ---"));

	int32 InitialFailed = FailedTests;

	// Test 1: Empty axiom should fail validation
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT(""));
		Gen->AddRuleSimple(TEXT("A"), TEXT("B"));

		FLSystemGenerationResult Result = Gen->Generate(1);

		bool bPassed = !Result.bSuccess;
		LogTestResult(TEXT("EmptyAxiomFails"), bPassed,
		              bPassed ? TEXT("") : TEXT("Should have failed"));
	}

	// Test 2: No rules should fail validation
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("ABC"));

		FLSystemGenerationResult Result = Gen->Generate(1);

		bool bPassed = !Result.bSuccess;
		LogTestResult(TEXT("NoRulesFails"), bPassed,
		              bPassed ? TEXT("") : TEXT("Should have failed"));
	}

	// Test 3: MaxStringLength enforcement
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Config.MaxStringLength = 50;
		Gen->Initialize(TEXT("F"));
		Gen->AddRuleSimple(TEXT("F"), TEXT("FF"));

		FString Result = Gen->GenerateString(10); // Would be 1024 F's without limit

		bool bPassed = Result.Len() <= 50;
		LogTestResult(TEXT("MaxStringLength"), bPassed,
		              FString::Printf(TEXT("Length: %d"), Result.Len()));
	}

	// Test 4: MaxIterations enforcement
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Config.MaxIterations = 3;
		Gen->Initialize(TEXT("A"));
		Gen->AddRuleSimple(TEXT("A"), TEXT("AA"));

		FLSystemGenerationResult Result = Gen->Generate(10); // Request 10, should stop at 3

		bool bPassed = Result.Stats.TotalIterations <= 3;
		LogTestResult(TEXT("MaxIterations"), bPassed,
		              FString::Printf(TEXT("Iterations: %d"), Result.Stats.TotalIterations));
	}

	// Test 5: Invalid rule predecessor (too long)
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("A"));

		FLSystemRule InvalidRule(TEXT("AB"), TEXT("X"), 1.0f); // Predecessor must be 1 char
		Gen->AddRule(InvalidRule);

		bool bPassed = Gen->GetRuleCount() == 0; // Invalid rule shouldn't be added
		LogTestResult(TEXT("InvalidRulePredecessor"), bPassed);
	}

	// Test 6: Statistics accuracy
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("A"));
		Gen->AddRuleSimple(TEXT("A"), TEXT("AB"));

		FLSystemGenerationResult Result = Gen->Generate(3);

		bool bPassed = (Result.Stats.TotalIterations == 3 &&
		                Result.Stats.FinalStringLength == Result.GeneratedString.Len());
		LogTestResult(TEXT("StatisticsAccuracy"), bPassed);
	}

	return FailedTests == InitialFailed;
}

// ============================================================================
// Known Patterns Tests
// ============================================================================

bool ATestLSystemGenerator::TestKnownPatterns()
{
	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("--- TestKnownPatterns ---"));

	int32 InitialFailed = FailedTests;

	// Test 1: Algae (Lindenmayer's original L-System)
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("A"));
		Gen->AddRuleSimple(TEXT("A"), TEXT("AB"));
		Gen->AddRuleSimple(TEXT("B"), TEXT("A"));

		// n=0: A
		// n=1: AB
		// n=2: ABA
		// n=3: ABAAB
		// n=4: ABAABABA
		// n=5: ABAABABAABAAB

		FString Result = Gen->GenerateString(5);
		VerifyOutput(Result, TEXT("ABAABABAABAAB"), TEXT("Algae_n5"));
	}

	// Test 2: Koch Curve
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("F"));
		Gen->AddRuleSimple(TEXT("F"), TEXT("F+F-F-F+F"));

		// n=1: F+F-F-F+F
		FString Result = Gen->GenerateString(1);
		VerifyOutput(Result, TEXT("F+F-F-F+F"), TEXT("KochCurve_n1"));
	}

	// Test 3: Sierpinski Triangle
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("F-G-G"));
		Gen->AddRuleSimple(TEXT("F"), TEXT("F-G+F+G-F"));
		Gen->AddRuleSimple(TEXT("G"), TEXT("GG"));

		// n=1: F-G+F+G-F-GG-GG
		FString Result = Gen->GenerateString(1);
		VerifyOutput(Result, TEXT("F-G+F+G-F-GG-GG"), TEXT("Sierpinski_n1"));
	}

	// Test 4: Binary Tree
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("0"));
		Gen->AddRuleSimple(TEXT("1"), TEXT("11"));
		Gen->AddRuleSimple(TEXT("0"), TEXT("1[0]0"));

		// n=1: 1[0]0
		FString Result = Gen->GenerateString(1);
		VerifyOutput(Result, TEXT("1[0]0"), TEXT("BinaryTree_n1"));
	}

	// Test 5: Dragon Curve
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("FX"));
		Gen->AddRuleSimple(TEXT("X"), TEXT("X+YF+"));
		Gen->AddRuleSimple(TEXT("Y"), TEXT("-FX-Y"));

		// n=1: FX+YF+
		FString Result = Gen->GenerateString(1);
		VerifyOutput(Result, TEXT("FX+YF+"), TEXT("DragonCurve_n1"));
	}

	return FailedTests == InitialFailed;
}

// ============================================================================
// Performance Tests
// ============================================================================

bool ATestLSystemGenerator::TestPerformance()
{
	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("--- TestPerformance ---"));

	int32 InitialFailed = FailedTests;

	// Test 1: Large string generation
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Config.MaxStringLength = 500000;
		Gen->Config.MaxIterations = 15;
		Gen->Config.bEnableDetailedLogging = false;
		Gen->Config.bStoreHistory = false;
		Gen->Initialize(TEXT("X"));
		Gen->AddRuleSimple(TEXT("X"), TEXT("F+[[X]-X]-F[-FX]+X"));
		Gen->AddRuleSimple(TEXT("F"), TEXT("FF"));

		FLSystemGenerationResult Result = Gen->Generate(PerformanceIterations);

		bool bPassed = Result.bSuccess && Result.Stats.GenerationTimeMs < 5000.0f;
		LogTestResult(TEXT("LargeStringGeneration"), bPassed,
		              FString::Printf(TEXT("Time: %.2fms, Length: %d, Iterations: %d"),
		                             Result.Stats.GenerationTimeMs,
		                             Result.Stats.FinalStringLength,
		                             Result.Stats.TotalIterations));
	}

	// Test 2: Many rules performance
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Config.bEnableDetailedLogging = false;
		Gen->Initialize(TEXT("ABCDEFGHIJ"));

		// Add many rules
		const TCHAR Symbols[] = TEXT("ABCDEFGHIJ");
		for (int32 i = 0; i < 10; ++i)
		{
			FString Pred = FString::Chr(Symbols[i]);
			FString Succ = Pred + Pred;
			Gen->AddRuleSimple(Pred, Succ);
		}

		FLSystemGenerationResult Result = Gen->Generate(5);

		bool bPassed = Result.bSuccess && Result.Stats.GenerationTimeMs < 1000.0f;
		LogTestResult(TEXT("ManyRulesPerformance"), bPassed,
		              FString::Printf(TEXT("Time: %.2fms, Rules: %d"),
		                             Result.Stats.GenerationTimeMs, Gen->GetRuleCount()));
	}

	// Test 3: Context-sensitive rule performance
	{
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Config.bEnableDetailedLogging = false;
		Gen->Initialize(TEXT("ABCABCABC"));
		Gen->AddContextRule(TEXT("A"), TEXT("B"), TEXT("C"), TEXT("XYZ"));
		Gen->AddRuleSimple(TEXT("A"), TEXT("AA"));

		FLSystemGenerationResult Result = Gen->Generate(6);

		bool bPassed = Result.bSuccess;
		LogTestResult(TEXT("ContextSensitivePerformance"), bPassed,
		              FString::Printf(TEXT("Time: %.2fms, Context rules applied: %d"),
		                             Result.Stats.GenerationTimeMs,
		                             Result.Stats.ContextRulesApplied));
	}

	return FailedTests == InitialFailed;
}

// ============================================================================
// Async Generation Tests
// ============================================================================

bool ATestLSystemGenerator::TestAsyncGeneration()
{
	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("--- TestAsyncGeneration ---"));

	// Note: This test is harder to run synchronously.
	// For Blueprint testing, you can bind to delegates.

	ULSystemGenerator* Gen = CreateTestGenerator();
	Gen->Initialize(TEXT("F"));
	Gen->AddRuleSimple(TEXT("F"), TEXT("FF"));

	// Just verify that async can be started
	Gen->GenerateAsync(3);

	bool bPassed = Gen->IsGenerating();
	LogTestResult(TEXT("AsyncStarted"), bPassed);

	// Cancel immediately for cleanup
	Gen->CancelAsyncGeneration();

	return bPassed;
}

// ============================================================================
// Phase 3: Turtle Interpreter Tests
// ============================================================================

bool ATestLSystemGenerator::TestTurtleInterpreter()
{
	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("--- TestTurtleInterpreter ---"));

	int32 InitialFailed = FailedTests;

	// Test 1: Simple forward movement creates segment
	{
		UTurtleInterpreter* Interp = NewObject<UTurtleInterpreter>(this);
		FTurtleConfig Config;
		Config.StepLength = 10.0f;
		Config.InitialWidth = 5.0f;

		TArray<FBranchSegment> Segments;
		TArray<FLeafData> Leaves;

		Interp->InterpretString(TEXT("F"), Config, Segments, Leaves);

		bool bPassed = Segments.Num() == 1;
		LogTestResult(TEXT("SingleForward"), bPassed,
		              FString::Printf(TEXT("Segments: %d"), Segments.Num()));
	}

	// Test 2: Multiple forwards create multiple segments
	{
		UTurtleInterpreter* Interp = NewObject<UTurtleInterpreter>(this);
		FTurtleConfig Config;

		TArray<FBranchSegment> Segments;
		TArray<FLeafData> Leaves;

		Interp->InterpretString(TEXT("FFF"), Config, Segments, Leaves);

		bool bPassed = Segments.Num() == 3;
		LogTestResult(TEXT("MultipleForwards"), bPassed,
		              FString::Printf(TEXT("Segments: %d"), Segments.Num()));
	}

	// Test 3: Branching creates correct segments
	{
		UTurtleInterpreter* Interp = NewObject<UTurtleInterpreter>(this);
		FTurtleConfig Config;

		TArray<FBranchSegment> Segments;
		TArray<FLeafData> Leaves;

		// F[F]F should create: trunk, branch, continuation
		Interp->InterpretString(TEXT("F[F]F"), Config, Segments, Leaves);

		bool bPassed = Segments.Num() == 3;
		LogTestResult(TEXT("SimpleBranching"), bPassed,
		              FString::Printf(TEXT("Segments: %d"), Segments.Num()));
	}

	// Test 4: Leaf placement
	{
		UTurtleInterpreter* Interp = NewObject<UTurtleInterpreter>(this);
		FTurtleConfig Config;

		TArray<FBranchSegment> Segments;
		TArray<FLeafData> Leaves;

		Interp->InterpretString(TEXT("FLF"), Config, Segments, Leaves);

		bool bPassed = Leaves.Num() == 1 && Segments.Num() == 2;
		LogTestResult(TEXT("LeafPlacement"), bPassed,
		              FString::Printf(TEXT("Segments: %d, Leaves: %d"), Segments.Num(), Leaves.Num()));
	}

	// Test 5: Rotation affects segment direction
	{
		UTurtleInterpreter* Interp = NewObject<UTurtleInterpreter>(this);
		FTurtleConfig Config;
		Config.DefaultAngle = 90.0f;
		Config.StepLength = 10.0f;

		TArray<FBranchSegment> Segments;
		TArray<FLeafData> Leaves;

		Interp->InterpretString(TEXT("F+F"), Config, Segments, Leaves);

		bool bPassed = Segments.Num() == 2;
		if (bPassed && Segments.Num() >= 2)
		{
			// After 90 degree rotation, directions should be different
			float DotProduct = FVector::DotProduct(Segments[0].Direction, Segments[1].Direction);
			bPassed = FMath::Abs(DotProduct) < 0.1f; // Should be ~perpendicular
		}
		LogTestResult(TEXT("RotationAffectsDirection"), bPassed);
	}

	// Test 6: Width falloff in branches
	{
		UTurtleInterpreter* Interp = NewObject<UTurtleInterpreter>(this);
		FTurtleConfig Config;
		Config.InitialWidth = 10.0f;
		Config.WidthFalloff = 0.5f;

		TArray<FBranchSegment> Segments;
		TArray<FLeafData> Leaves;

		Interp->InterpretString(TEXT("F[F]"), Config, Segments, Leaves);

		bool bPassed = Segments.Num() == 2;
		if (bPassed)
		{
			// Branch segment should have smaller width
			bPassed = Segments[1].StartRadius < Segments[0].StartRadius;
		}
		LogTestResult(TEXT("WidthFalloff"), bPassed);
	}

	// Test 7: Max depth tracking
	{
		UTurtleInterpreter* Interp = NewObject<UTurtleInterpreter>(this);
		FTurtleConfig Config;

		TArray<FBranchSegment> Segments;
		TArray<FLeafData> Leaves;

		Interp->InterpretString(TEXT("F[[F]F]F"), Config, Segments, Leaves);

		bool bPassed = Interp->GetMaxDepth() == 2;
		LogTestResult(TEXT("MaxDepthTracking"), bPassed,
		              FString::Printf(TEXT("MaxDepth: %d"), Interp->GetMaxDepth()));
	}

	return FailedTests == InitialFailed;
}

// ============================================================================
// Phase 3: Tree Geometry Tests
// ============================================================================

bool ATestLSystemGenerator::TestTreeGeometry()
{
	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("--- TestTreeGeometry ---"));

	int32 InitialFailed = FailedTests;

	// Test 1: Single segment generates vertices
	{
		UTreeGeometry* Geo = NewObject<UTreeGeometry>(this);

		TArray<FBranchSegment> Segments;
		FBranchSegment Seg;
		Seg.StartPosition = FVector::ZeroVector;
		Seg.EndPosition = FVector(0, 0, 100);
		Seg.StartRadius = 10.0f;
		Seg.EndRadius = 8.0f;
		Seg.Direction = FVector::UpVector;
		Segments.Add(Seg);

		TArray<FLeafData> Leaves;

		FTreeMeshData MeshData = Geo->GenerateMesh(Segments, Leaves, 8, false);

		bool bPassed = MeshData.Vertices.Num() > 0 && MeshData.Triangles.Num() > 0;
		LogTestResult(TEXT("SingleSegmentGeometry"), bPassed,
		              FString::Printf(TEXT("Verts: %d, Tris: %d"),
		                             MeshData.Vertices.Num(), MeshData.GetTriangleCount()));
	}

	// Test 2: Radial segments affect vertex count
	{
		UTreeGeometry* Geo = NewObject<UTreeGeometry>(this);

		TArray<FBranchSegment> Segments;
		FBranchSegment Seg;
		Seg.StartPosition = FVector::ZeroVector;
		Seg.EndPosition = FVector(0, 0, 100);
		Seg.StartRadius = 10.0f;
		Seg.EndRadius = 8.0f;
		Seg.Direction = FVector::UpVector;
		Segments.Add(Seg);

		TArray<FLeafData> Leaves;

		FTreeMeshData Mesh4 = Geo->GenerateMesh(Segments, Leaves, 4, false);
		FTreeMeshData Mesh8 = Geo->GenerateMesh(Segments, Leaves, 8, false);

		bool bPassed = Mesh8.Vertices.Num() > Mesh4.Vertices.Num();
		LogTestResult(TEXT("RadialSegmentsAffectVerts"), bPassed,
		              FString::Printf(TEXT("4-seg: %d verts, 8-seg: %d verts"),
		                             Mesh4.Vertices.Num(), Mesh8.Vertices.Num()));
	}

	// Test 3: Leaf geometry generation
	{
		UTreeGeometry* Geo = NewObject<UTreeGeometry>(this);
		Geo->DefaultLeafSize = FVector2D(10.0f, 15.0f);

		TArray<FBranchSegment> Segments;
		TArray<FLeafData> Leaves;

		FLeafData Leaf;
		Leaf.Position = FVector(0, 0, 50);
		Leaf.Normal = FVector::UpVector;
		Leaf.UpDirection = FVector::ForwardVector;
		Leaf.Size = FVector2D(10.0f, 15.0f);
		Leaves.Add(Leaf);

		FTreeMeshData MeshData = Geo->GenerateMesh(Segments, Leaves, 8, true);

		// Each leaf is a quad = 4 vertices, 4 triangles (2 front + 2 back)
		bool bPassed = MeshData.Vertices.Num() >= 4;
		LogTestResult(TEXT("LeafGeometry"), bPassed,
		              FString::Printf(TEXT("Verts: %d, Tris: %d"),
		                             MeshData.Vertices.Num(), MeshData.GetTriangleCount()));
	}

	// Test 4: LOD generation
	{
		UTreeGeometry* Geo = NewObject<UTreeGeometry>(this);

		TArray<FBranchSegment> Segments;
		FBranchSegment Seg;
		Seg.StartPosition = FVector::ZeroVector;
		Seg.EndPosition = FVector(0, 0, 100);
		Seg.StartRadius = 10.0f;
		Seg.EndRadius = 8.0f;
		Seg.Direction = FVector::UpVector;
		Segments.Add(Seg);

		TArray<FLeafData> Leaves;
		FLeafData Leaf;
		Leaf.Position = FVector(0, 0, 100);
		Leaf.Normal = FVector::UpVector;
		Leaf.UpDirection = FVector::ForwardVector;
		Leaf.Size = FVector2D(10.0f, 15.0f);
		Leaves.Add(Leaf);

		TArray<FTreeLODLevel> LODLevels;
		FTreeLODLevel LOD0;
		LOD0.RadialSegments = 12;
		LOD0.bIncludeLeaves = true;
		LODLevels.Add(LOD0);

		FTreeLODLevel LOD1;
		LOD1.RadialSegments = 6;
		LOD1.bIncludeLeaves = false;
		LODLevels.Add(LOD1);

		TArray<FTreeMeshData> LODs = Geo->GenerateMeshLODs(Segments, Leaves, LODLevels);

		bool bPassed = LODs.Num() == 2 &&
		               LODs[0].Vertices.Num() > LODs[1].Vertices.Num();
		LogTestResult(TEXT("LODGeneration"), bPassed,
		              FString::Printf(TEXT("%d LODs generated"), LODs.Num()));
	}

	// Test 5: UV generation
	{
		UTreeGeometry* Geo = NewObject<UTreeGeometry>(this);

		TArray<FBranchSegment> Segments;
		FBranchSegment Seg;
		Seg.StartPosition = FVector::ZeroVector;
		Seg.EndPosition = FVector(0, 0, 100);
		Seg.StartRadius = 10.0f;
		Seg.EndRadius = 8.0f;
		Seg.Direction = FVector::UpVector;
		Segments.Add(Seg);

		TArray<FLeafData> Leaves;

		FTreeMeshData MeshData = Geo->GenerateMesh(Segments, Leaves, 8, false);

		bool bPassed = MeshData.UVs.Num() == MeshData.Vertices.Num();
		LogTestResult(TEXT("UVGeneration"), bPassed,
		              FString::Printf(TEXT("UVs: %d, Verts: %d"),
		                             MeshData.UVs.Num(), MeshData.Vertices.Num()));
	}

	return FailedTests == InitialFailed;
}

// ============================================================================
// Phase 3: Tree Math Tests
// ============================================================================

bool ATestLSystemGenerator::TestTreeMath()
{
	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("--- TestTreeMath ---"));

	int32 InitialFailed = FailedTests;

	// Test 1: Rotate vector around axis
	{
		FVector Result = UTreeMath::RotateVectorAroundAxis(FVector::ForwardVector, FVector::UpVector, 90.0f);

		bool bPassed = FMath::IsNearlyEqual(Result.X, 0.0f, 0.01f) &&
		               FMath::IsNearlyEqual(FMath::Abs(Result.Y), 1.0f, 0.01f);
		LogTestResult(TEXT("RotateVectorAroundAxis"), bPassed,
		              FString::Printf(TEXT("Result: (%.2f, %.2f, %.2f)"), Result.X, Result.Y, Result.Z));
	}

	// Test 2: 360 degree rotation returns to original
	{
		FVector Original = FVector(1.0f, 2.0f, 3.0f).GetSafeNormal();
		FVector Result = UTreeMath::RotateVectorAroundAxis(Original, FVector::UpVector, 360.0f);

		bool bPassed = Original.Equals(Result, 0.01f);
		LogTestResult(TEXT("FullRotationReturns"), bPassed);
	}

	// Test 3: Get perpendicular vectors
	{
		FVector Direction = FVector::UpVector;
		FVector Right, Up;
		UTreeMath::GetPerpendicularVectors(Direction, Right, Up);

		bool bRightPerp = FMath::IsNearlyZero(FVector::DotProduct(Direction, Right), 0.01f);
		bool bUpPerp = FMath::IsNearlyZero(FVector::DotProduct(Direction, Up), 0.01f);
		bool bRightUpPerp = FMath::IsNearlyZero(FVector::DotProduct(Right, Up), 0.01f);

		bool bPassed = bRightPerp && bUpPerp && bRightUpPerp;
		LogTestResult(TEXT("GetPerpendicularVectors"), bPassed);
	}

	// Test 4: Reorthogonalize basis
	{
		FVector Forward = FVector(1.0f, 0.1f, 0.0f); // Slightly off
		FVector Left = FVector(0.1f, 1.0f, 0.0f);
		FVector Up = FVector(0.0f, 0.0f, 1.0f);

		UTreeMath::ReorthogonalizeBasis(Forward, Left, Up);

		bool bNormalized = FMath::IsNearlyEqual(Forward.Size(), 1.0f, 0.01f) &&
		                   FMath::IsNearlyEqual(Left.Size(), 1.0f, 0.01f) &&
		                   FMath::IsNearlyEqual(Up.Size(), 1.0f, 0.01f);
		bool bOrthogonal = FMath::IsNearlyZero(FVector::DotProduct(Forward, Left), 0.01f) &&
		                   FMath::IsNearlyZero(FVector::DotProduct(Forward, Up), 0.01f) &&
		                   FMath::IsNearlyZero(FVector::DotProduct(Left, Up), 0.01f);

		bool bPassed = bNormalized && bOrthogonal;
		LogTestResult(TEXT("ReorthogonalizeBasis"), bPassed);
	}

	// Test 5: Calculate child width (Leonardo's rule)
	{
		float ParentWidth = 10.0f;
		float ChildWidth = UTreeMath::CalculateChildWidth(ParentWidth, 2, 2.0f);

		// With exponent 2 and 2 children: child = parent / sqrt(2) ~= 7.07
		bool bPassed = FMath::IsNearlyEqual(ChildWidth, 7.07f, 0.1f);
		LogTestResult(TEXT("CalculateChildWidth"), bPassed,
		              FString::Printf(TEXT("Parent: %.2f, Child: %.2f"), ParentWidth, ChildWidth));
	}

	// Test 6: Apply tropism
	{
		FVector Direction = FVector::UpVector;
		FVector Tropism = FVector(0, 0, -1); // Gravity down
		FVector Result = UTreeMath::ApplyTropism(Direction, Tropism, 0.5f);

		// Should bend toward gravity
		bool bPassed = Result.Z < 1.0f && Result.IsNormalized();
		LogTestResult(TEXT("ApplyTropism"), bPassed,
		              FString::Printf(TEXT("Result: (%.2f, %.2f, %.2f)"), Result.X, Result.Y, Result.Z));
	}

	// Test 7: Generate ring points
	{
		TArray<FVector> Points = UTreeMath::GenerateRingPoints(FVector::ZeroVector, FVector::UpVector, 10.0f, 8);

		bool bCorrectCount = Points.Num() == 8;
		bool bCorrectRadius = true;
		for (const FVector& Point : Points)
		{
			float Distance = FVector::Dist(Point, FVector::ZeroVector);
			if (!FMath::IsNearlyEqual(Distance, 10.0f, 0.1f))
			{
				bCorrectRadius = false;
				break;
			}
		}

		bool bPassed = bCorrectCount && bCorrectRadius;
		LogTestResult(TEXT("GenerateRingPoints"), bPassed,
		              FString::Printf(TEXT("Points: %d"), Points.Num()));
	}

	return FailedTests == InitialFailed;
}

// ============================================================================
// Phase 3: Full Pipeline Test
// ============================================================================

bool ATestLSystemGenerator::TestFullPipeline()
{
	UE_LOG(LogLSystem, Log, TEXT(""));
	UE_LOG(LogLSystem, Log, TEXT("--- TestFullPipeline ---"));

	int32 InitialFailed = FailedTests;

	// Test: Full tree generation pipeline
	{
		// Step 1: Generate L-System string
		ULSystemGenerator* Gen = CreateTestGenerator();
		Gen->Initialize(TEXT("F"));
		Gen->AddRuleSimple(TEXT("F"), TEXT("FF-[-F+F+FL]+[+F-F-FL]"));
		Gen->SetRandomSeed(42);

		FLSystemGenerationResult GenResult = Gen->Generate(3);

		if (!GenResult.bSuccess)
		{
			LogTestResult(TEXT("FullPipeline_LSystemGen"), false, TEXT("L-System generation failed"));
			return false;
		}

		UE_LOG(LogLSystem, Verbose, TEXT("Generated string length: %d"), GenResult.GeneratedString.Len());

		// Step 2: Interpret with turtle
		UTurtleInterpreter* Interp = NewObject<UTurtleInterpreter>(this);
		FTurtleConfig Config;
		Config.DefaultAngle = 25.0f;
		Config.StepLength = 10.0f;
		Config.InitialWidth = 5.0f;
		Config.WidthFalloff = 0.7f;
		Config.RandomSeed = 42;

		TArray<FBranchSegment> Segments;
		TArray<FLeafData> Leaves;

		Interp->InterpretString(GenResult.GeneratedString, Config, Segments, Leaves);

		bool bInterpOk = Segments.Num() > 0;
		LogTestResult(TEXT("FullPipeline_Interpretation"), bInterpOk,
		              FString::Printf(TEXT("Segments: %d, Leaves: %d"), Segments.Num(), Leaves.Num()));

		if (!bInterpOk)
		{
			return false;
		}

		// Step 3: Generate geometry
		UTreeGeometry* Geo = NewObject<UTreeGeometry>(this);

		TArray<FTreeLODLevel> LODLevels;
		FTreeLODLevel LOD0;
		LOD0.RadialSegments = 8;
		LOD0.bIncludeLeaves = true;
		LODLevels.Add(LOD0);

		TArray<FTreeMeshData> LODs = Geo->GenerateMeshLODs(Segments, Leaves, LODLevels);

		bool bGeoOk = LODs.Num() > 0 && LODs[0].Vertices.Num() > 0;
		LogTestResult(TEXT("FullPipeline_Geometry"), bGeoOk,
		              FString::Printf(TEXT("Vertices: %d, Triangles: %d"),
		                             LODs[0].Vertices.Num(), LODs[0].GetTriangleCount()));

		// Overall pipeline success
		bool bPassed = GenResult.bSuccess && bInterpOk && bGeoOk;
		LogTestResult(TEXT("FullPipeline_Complete"), bPassed);
	}

	return FailedTests == InitialFailed;
}

// ============================================================================
// Helper Methods
// ============================================================================

void ATestLSystemGenerator::LogTestResult(const FString& TestName, bool bPassed, const FString& Details)
{
	if (bPassed)
	{
		PassedTests++;
		UE_LOG(LogLSystem, Log, TEXT("[PASS] %s %s"), *TestName,
		       Details.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("- %s"), *Details));
	}
	else
	{
		FailedTests++;
		UE_LOG(LogLSystem, Error, TEXT("[FAIL] %s %s"), *TestName,
		       Details.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("- %s"), *Details));
	}
}

bool ATestLSystemGenerator::VerifyOutput(const FString& Actual, const FString& Expected, const FString& TestName)
{
	bool bPassed = (Actual == Expected);

	if (!bPassed && bVerboseLogging)
	{
		UE_LOG(LogLSystem, Warning, TEXT("%s: Expected '%s' but got '%s'"),
		       *TestName, *Expected, *Actual);
	}

	LogTestResult(TestName, bPassed);
	return bPassed;
}

ULSystemGenerator* ATestLSystemGenerator::CreateTestGenerator()
{
	return NewObject<ULSystemGenerator>(this);
}

void ATestLSystemGenerator::ResetTestCounters()
{
	PassedTests = 0;
	FailedTests = 0;
	TotalTestTimeMs = 0.0f;
}
