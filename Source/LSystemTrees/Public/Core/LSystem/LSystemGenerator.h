// LSystemGenerator.h
// Core L-System string generator with async and context-sensitive support
// Part of LSystemTrees Plugin - Phase 2

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/LSystem/LSystemTypes.h"
#include "LSystemGenerator.generated.h"

// Forward declarations
class FLSystemAsyncTask;

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogLSystem, Log, All);

/**
 * L-System string generator with stochastic and context-sensitive rule support.
 *
 * An L-System (Lindenmayer System) is a parallel rewriting system used to model
 * plant growth, fractals, and other recursive structures. This generator takes an
 * axiom (initial string) and applies production rules iteratively to generate
 * complex strings.
 *
 * Features:
 *   - Context-free rules: F -> FF
 *   - Context-sensitive rules: A < B > C -> X
 *   - Stochastic rules: F -> FF (p=0.5)
 *   - Async generation with progress callbacks
 *   - Detailed logging and statistics
 *   - Blueprint integration
 *
 * Example Usage (C++):
 *   ULSystemGenerator* Gen = NewObject<ULSystemGenerator>();
 *   Gen->Initialize(TEXT("F"));
 *   Gen->AddRuleSimple(TEXT("F"), TEXT("F+F-F-F+F"));
 *   FLSystemGenerationResult Result = Gen->Generate(4);
 *
 * Example Usage (Blueprint):
 *   Create generator -> Initialize -> Add Rules -> Generate
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "L-System Generator"))
class LSYSTEMTREES_API ULSystemGenerator : public UObject
{
	GENERATED_BODY()

public:
	ULSystemGenerator();
	virtual ~ULSystemGenerator();

	// ========================================================================
	// Configuration Properties
	// ========================================================================

	/** The initial string (axiom) for the L-System */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Configuration")
	FString CurrentAxiom;

	/** Production rules for the L-System */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Configuration")
	TArray<FLSystemRule> Rules;

	/** Configuration settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LSystem|Configuration")
	FLSystemConfig Config;

	// ========================================================================
	// Delegates (for async and event handling)
	// ========================================================================

	/** Delegate fired when async generation completes */
	UPROPERTY(BlueprintAssignable, Category = "LSystem|Events")
	FOnLSystemGenerationComplete OnGenerationComplete;

	/** Delegate fired after each iteration completes */
	UPROPERTY(BlueprintAssignable, Category = "LSystem|Events")
	FOnLSystemIterationComplete OnIterationComplete;

	/** Delegate fired to report generation progress */
	UPROPERTY(BlueprintAssignable, Category = "LSystem|Events")
	FOnLSystemGenerationProgress OnGenerationProgress;

	// ========================================================================
	// Initialization Methods
	// ========================================================================

	/**
	 * Initialize the generator with an axiom.
	 * @param Axiom The initial string to start generation from
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Setup",
		meta = (DisplayName = "Initialize", Keywords = "setup start begin axiom"))
	void Initialize(const FString& Axiom);

	/**
	 * Reset the generator to initial state.
	 * Clears axiom, rules, and all state.
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Setup")
	void Reset();

	// ========================================================================
	// Rule Management Methods
	// ========================================================================

	/**
	 * Add a production rule.
	 * @param Rule The rule to add
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Rules")
	void AddRule(const FLSystemRule& Rule);

	/**
	 * Add a simple rule (probability = 1.0, no context).
	 * @param Predecessor Symbol to replace (single character)
	 * @param Successor Replacement string
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Rules",
		meta = (DisplayName = "Add Simple Rule"))
	void AddRuleSimple(const FString& Predecessor, const FString& Successor);

	/**
	 * Add a stochastic rule with probability.
	 * @param Predecessor Symbol to replace (single character)
	 * @param Successor Replacement string
	 * @param Probability Probability of rule being selected (0.0 - 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Rules",
		meta = (DisplayName = "Add Stochastic Rule"))
	void AddRuleStochastic(const FString& Predecessor, const FString& Successor, float Probability);

	/**
	 * Add a context-sensitive rule.
	 * @param LeftContext Symbol that must precede predecessor (empty = any)
	 * @param Predecessor Symbol to replace (single character)
	 * @param RightContext Symbol that must follow predecessor (empty = any)
	 * @param Successor Replacement string
	 * @param Probability Probability of rule being selected (0.0 - 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Rules",
		meta = (DisplayName = "Add Context Rule"))
	void AddContextRule(const FString& LeftContext, const FString& Predecessor,
	                    const FString& RightContext, const FString& Successor,
	                    float Probability = 1.0f);

	/**
	 * Remove all rules for a given predecessor.
	 * @param Predecessor The predecessor symbol to remove rules for
	 * @return True if any rules were removed
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Rules")
	bool RemoveRule(const FString& Predecessor);

	/**
	 * Remove a specific rule.
	 * @param Rule The exact rule to remove
	 * @return True if the rule was found and removed
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Rules")
	bool RemoveSpecificRule(const FLSystemRule& Rule);

	/**
	 * Clear all rules.
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Rules")
	void ClearRules();

	/**
	 * Get the number of rules.
	 * @return Number of rules currently defined
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules")
	int32 GetRuleCount() const;

	/**
	 * Check if a rule exists for the given symbol.
	 * @param Symbol The symbol to check
	 * @return True if at least one rule exists for this symbol
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules")
	bool HasRuleForSymbol(const FString& Symbol) const;

	// ========================================================================
	// Generation Methods (Synchronous)
	// ========================================================================

	/**
	 * Generate L-System string with full result information.
	 * This is a synchronous (blocking) operation.
	 * @param Iterations Number of iterations to perform
	 * @return FLSystemGenerationResult containing the result, statistics, and any errors
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Generation",
		meta = (DisplayName = "Generate"))
	FLSystemGenerationResult Generate(int32 Iterations);

	/**
	 * Simple generation that returns just the string.
	 * This is a synchronous (blocking) operation.
	 * @param Iterations Number of iterations to perform
	 * @return Generated string, or empty string on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Generation",
		meta = (DisplayName = "Generate String"))
	FString GenerateString(int32 Iterations);

	/**
	 * Perform a single iteration on the given string.
	 * Useful for step-by-step debugging.
	 * @param InputString String to process
	 * @return Processed string after one iteration
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Generation")
	FString PerformSingleIteration(const FString& InputString);

	// ========================================================================
	// Generation Methods (Asynchronous)
	// ========================================================================

	/**
	 * Start async generation.
	 * Results will be delivered via OnGenerationComplete delegate.
	 * Progress updates via OnGenerationProgress delegate.
	 * @param Iterations Number of iterations to perform
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Generation",
		meta = (DisplayName = "Generate Async"))
	void GenerateAsync(int32 Iterations);

	/**
	 * Cancel ongoing async generation.
	 * OnGenerationComplete will fire with bSuccess = false.
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Generation")
	void CancelAsyncGeneration();

	/**
	 * Check if async generation is currently in progress.
	 * @return True if generation is running
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Generation")
	bool IsGenerating() const;

	// ========================================================================
	// State and Statistics
	// ========================================================================

	/**
	 * Get current generation state.
	 * @return Current state including iteration count and progress
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|State")
	FLSystemState GetCurrentState() const;

	/**
	 * Get statistics from last generation.
	 * @return Statistics including timing, rule applications, etc.
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|State")
	FLSystemStatistics GetStatistics() const;

	/**
	 * Check if generator is properly configured.
	 * @return True if axiom and at least one rule are defined
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|State")
	bool IsValid() const;

	/**
	 * Validate current configuration and get detailed error if invalid.
	 * @param OutError Detailed error message if validation fails
	 * @return True if configuration is valid
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|State")
	bool Validate(FString& OutError) const;

	// ========================================================================
	// Utility Methods
	// ========================================================================

	/**
	 * Set random seed for reproducible stochastic results.
	 * @param Seed The seed value (0 = use time-based seed)
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Configuration")
	void SetRandomSeed(int32 Seed);

	/**
	 * Get estimated string length after N iterations (approximate).
	 * @param Iterations Number of iterations to estimate for
	 * @return Estimated length (may be inaccurate for stochastic systems)
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Utility")
	int32 EstimateStringLength(int32 Iterations) const;

	/**
	 * Count occurrences of each symbol in a string.
	 * @param InputString The string to analyze
	 * @return Map of symbol -> count
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Utility")
	static TMap<FString, int32> CountSymbols(const FString& InputString);

protected:
	// ========================================================================
	// Internal Methods
	// ========================================================================

	/**
	 * Apply all rules to transform the input string (one iteration).
	 * @param InputString String to transform
	 * @return Transformed string
	 */
	FString ApplyRules(const FString& InputString);

	/**
	 * Select a rule for the given symbol and context.
	 * Handles context-sensitive matching and stochastic selection.
	 * @param Symbol The symbol to find a rule for
	 * @param LeftContext The character before the symbol (or '\0')
	 * @param RightContext The character after the symbol (or '\0')
	 * @return Pointer to selected rule, or nullptr if no rule matches
	 */
	const FLSystemRule* SelectRule(TCHAR Symbol, TCHAR LeftContext, TCHAR RightContext);

	/**
	 * Build lookup tables for faster rule access.
	 * Call this after rules are modified.
	 */
	void BuildRuleLookup();

	/**
	 * Check if generation should terminate.
	 * @param CurrentString Current string state
	 * @param Iteration Current iteration number
	 * @param OutReason Reason for termination if returning true
	 * @return True if generation should stop
	 */
	bool CheckTermination(const FString& CurrentString, int32 Iteration, FString& OutReason) const;

	/**
	 * Update statistics after generation.
	 * @param FinalString The final generated string
	 * @param Iterations Number of iterations completed
	 * @param StartTime Time when generation started
	 */
	void UpdateStatistics(const FString& FinalString, int32 Iterations, double StartTime);

	/**
	 * Calculate symbol counts for a string.
	 * @param InputString String to analyze
	 */
	void CalculateSymbolCounts(const FString& InputString);

	/**
	 * Log iteration details (if detailed logging is enabled).
	 * @param Iteration Current iteration number
	 * @param CurrentString Current string state
	 */
	void LogIteration(int32 Iteration, const FString& CurrentString);

	/**
	 * Perform the actual generation work.
	 * Used by both sync and async paths.
	 * @param Iterations Number of iterations
	 * @param bAsync Whether this is an async call
	 * @return Generation result
	 */
	FLSystemGenerationResult DoGeneration(int32 Iterations, bool bAsync = false);

	/**
	 * Handle completion of async generation.
	 * Called from the async task when done.
	 * @param Result The generation result
	 */
	void HandleAsyncComplete(const FLSystemGenerationResult& Result);

private:
	/** Current generation state */
	FLSystemState State;

	/** Statistics from last generation */
	FLSystemStatistics Statistics;

	/** Random stream for stochastic rule selection */
	FRandomStream RandomStream;

	/** Lookup table: Predecessor char -> Array of applicable rules */
	TMap<TCHAR, TArray<const FLSystemRule*>> RuleLookup;

	/** Cached total probabilities for each predecessor (for normalization) */
	TMap<TCHAR, float> ProbabilityTotals;

	/** Flag indicating if lookup needs rebuilding */
	bool bLookupDirty;

	/** Flag for cancelling async generation */
	TAtomic<bool> bCancelRequested;

	/** The async task (if running) */
	FLSystemAsyncTask* CurrentAsyncTask;

	/** Thread for async generation */
	FRunnableThread* CurrentAsyncThread;

	/** Critical section for thread-safe state access */
	mutable FCriticalSection StateLock;

	// Friend class for async task access
	friend class FLSystemAsyncTask;
};
