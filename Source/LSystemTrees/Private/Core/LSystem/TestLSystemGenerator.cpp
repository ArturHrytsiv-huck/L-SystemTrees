// TestLSystemGenerator.cpp
// Test actor for validating L-System generator functionality
// Part of LSystemTrees Plugin - Phase 2

#include "Core/LSystem/TestLSystemGenerator.h"
#include "Core/LSystem/LSystemRule.h"

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
