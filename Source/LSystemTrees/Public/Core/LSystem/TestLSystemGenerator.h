// TestLSystemGenerator.h
// Test actor for validating L-System generator functionality
// Part of LSystemTrees Plugin - Phase 2

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/LSystem/LSystemGenerator.h"
#include "TestLSystemGenerator.generated.h"

/**
 * Test actor for validating L-System generator functionality.
 * Place in a level and call RunAllTests() from Blueprint or C++.
 *
 * Test Categories:
 *   - Simple Generation: Basic rule application
 *   - Stochastic Generation: Random rule selection
 *   - Context-Sensitive: Context-aware rules
 *   - Edge Cases: Error handling and limits
 *   - Known Patterns: Verified L-System patterns
 *   - Performance: Timing and benchmarks
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "L-System Test Actor"))
class LSYSTEMTREES_API ATestLSystemGenerator : public AActor
{
	GENERATED_BODY()

public:
	ATestLSystemGenerator();

	// ========================================================================
	// Test Configuration
	// ========================================================================

	/** Whether to log detailed test output */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Testing")
	bool bVerboseLogging;

	/** Number of iterations for performance tests */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Testing",
		meta = (ClampMin = "1", ClampMax = "15"))
	int32 PerformanceIterations;

	/** Run tests automatically when actor begins play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Testing")
	bool bRunTestsOnBeginPlay;

	// ========================================================================
	// Test Results (read-only)
	// ========================================================================

	/** Number of tests that passed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Testing|Results")
	int32 PassedTests;

	/** Number of tests that failed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Testing|Results")
	int32 FailedTests;

	/** Total test execution time in milliseconds */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Testing|Results")
	float TotalTestTimeMs;

	// ========================================================================
	// Test Methods
	// ========================================================================

	/**
	 * Run all tests and return overall success.
	 * @return True if all tests passed
	 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool RunAllTests();

	/**
	 * Test basic L-System generation.
	 * @return True if all simple generation tests passed
	 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool TestSimpleGeneration();

	/**
	 * Test stochastic rule selection.
	 * @return True if all stochastic tests passed
	 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool TestStochasticGeneration();

	/**
	 * Test context-sensitive rules.
	 * @return True if all context-sensitive tests passed
	 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool TestContextSensitive();

	/**
	 * Test edge cases and error handling.
	 * @return True if all edge case tests passed
	 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool TestEdgeCases();

	/**
	 * Test known L-System patterns (Koch curve, etc.).
	 * @return True if all known pattern tests passed
	 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool TestKnownPatterns();

	/**
	 * Test generation performance.
	 * @return True if performance is acceptable
	 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool TestPerformance();

	/**
	 * Test async generation.
	 * @return True if async tests passed
	 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool TestAsyncGeneration();

protected:
	virtual void BeginPlay() override;

private:
	// ========================================================================
	// Helper Methods
	// ========================================================================

	/** Log a test result */
	void LogTestResult(const FString& TestName, bool bPassed, const FString& Details = TEXT(""));

	/** Verify that a string matches expected output */
	bool VerifyOutput(const FString& Actual, const FString& Expected, const FString& TestName);

	/** Create a new generator for testing */
	ULSystemGenerator* CreateTestGenerator();

	/** Reset test counters */
	void ResetTestCounters();
};
