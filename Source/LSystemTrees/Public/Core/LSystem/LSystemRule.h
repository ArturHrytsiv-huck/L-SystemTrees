// LSystemRule.h
// Blueprint function library for L-System rule operations
// Part of LSystemTrees Plugin - Phase 2

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/LSystem/LSystemTypes.h"
#include "LSystemRule.generated.h"

/**
 * Blueprint function library for L-System rule operations.
 * Provides static helper functions for creating, validating, and manipulating L-System rules.
 */
UCLASS()
class LSYSTEMTREES_API ULSystemRuleLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ========================================================================
	// Rule Creation
	// ========================================================================

	/**
	 * Create a simple rule with default probability 1.0.
	 * @param Predecessor The symbol to replace (single character)
	 * @param Successor The replacement string
	 * @return A new L-System rule
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Make Simple Rule", Keywords = "create new"))
	static FLSystemRule MakeSimpleRule(const FString& Predecessor, const FString& Successor);

	/**
	 * Create a stochastic rule with specified probability.
	 * @param Predecessor The symbol to replace (single character)
	 * @param Successor The replacement string
	 * @param Probability Probability of this rule being selected (0.0 - 1.0)
	 * @return A new stochastic L-System rule
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Make Stochastic Rule", Keywords = "create new random"))
	static FLSystemRule MakeStochasticRule(const FString& Predecessor,
	                                        const FString& Successor,
	                                        float Probability);

	/**
	 * Create a context-sensitive rule.
	 * Context format: LeftContext < Predecessor > RightContext -> Successor
	 * @param LeftContext Symbol that must appear before predecessor (empty = any)
	 * @param Predecessor The symbol to replace (single character)
	 * @param RightContext Symbol that must appear after predecessor (empty = any)
	 * @param Successor The replacement string
	 * @param Probability Probability of this rule being selected (0.0 - 1.0)
	 * @return A new context-sensitive L-System rule
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Make Context Rule", Keywords = "create new context sensitive"))
	static FLSystemRule MakeContextRule(const FString& LeftContext,
	                                     const FString& Predecessor,
	                                     const FString& RightContext,
	                                     const FString& Successor,
	                                     float Probability = 1.0f);

	/**
	 * Parse a rule from string notation.
	 * Supports formats:
	 *   "F -> FF"              Simple rule
	 *   "F -> FF (0.5)"        Stochastic rule
	 *   "A < B -> X"           Left context
	 *   "B > C -> X"           Right context
	 *   "A < B > C -> X"       Full context
	 *   "A < B > C -> X (0.5)" Full context with probability
	 *
	 * @param RuleString The rule in string notation
	 * @param OutRule The parsed rule (if successful)
	 * @param OutError Error message (if parsing failed)
	 * @return True if parsing was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Rules",
		meta = (DisplayName = "Parse Rule String", Keywords = "parse read"))
	static bool ParseRuleString(const FString& RuleString,
	                            FLSystemRule& OutRule,
	                            FString& OutError);

	// ========================================================================
	// Rule Validation
	// ========================================================================

	/**
	 * Validate a rule and return error message if invalid.
	 * @param Rule The rule to validate
	 * @param OutError Error description if validation fails
	 * @return True if the rule is valid
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Validate Rule"))
	static bool ValidateRule(const FLSystemRule& Rule, FString& OutError);

	/**
	 * Check if a rule is valid (simple check without error message).
	 * @param Rule The rule to check
	 * @return True if the rule is valid
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Is Rule Valid"))
	static bool IsRuleValid(const FLSystemRule& Rule);

	/**
	 * Validate an array of rules.
	 * @param Rules Array of rules to validate
	 * @param OutErrors Array of error messages for invalid rules (indexed by rule index)
	 * @return True if all rules are valid
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Rules",
		meta = (DisplayName = "Validate Rules Array"))
	static bool ValidateRulesArray(const TArray<FLSystemRule>& Rules,
	                               TArray<FString>& OutErrors);

	// ========================================================================
	// Rule Manipulation
	// ========================================================================

	/**
	 * Normalize probabilities for rules with the same predecessor.
	 * Ensures that for each predecessor, all rule probabilities sum to 1.0.
	 * @param Rules Array of rules to normalize (modified in place)
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Rules",
		meta = (DisplayName = "Normalize Probabilities"))
	static void NormalizeProbabilities(UPARAM(ref) TArray<FLSystemRule>& Rules);

	/**
	 * Get all rules that match a specific predecessor.
	 * @param Rules Source array of rules
	 * @param Predecessor The predecessor to search for
	 * @return Array of matching rules
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Get Rules For Predecessor"))
	static TArray<FLSystemRule> GetRulesForPredecessor(const TArray<FLSystemRule>& Rules,
	                                                    const FString& Predecessor);

	/**
	 * Get all context-sensitive rules from an array.
	 * @param Rules Source array of rules
	 * @return Array of context-sensitive rules only
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Get Context-Sensitive Rules"))
	static TArray<FLSystemRule> GetContextSensitiveRules(const TArray<FLSystemRule>& Rules);

	/**
	 * Get all context-free (simple) rules from an array.
	 * @param Rules Source array of rules
	 * @return Array of context-free rules only
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Get Context-Free Rules"))
	static TArray<FLSystemRule> GetContextFreeRules(const TArray<FLSystemRule>& Rules);

	/**
	 * Sort rules by context specificity (more specific rules first).
	 * This is important for proper context-sensitive rule matching.
	 * @param Rules Array of rules to sort (modified in place)
	 */
	UFUNCTION(BlueprintCallable, Category = "LSystem|Rules",
		meta = (DisplayName = "Sort Rules By Specificity"))
	static void SortRulesBySpecificity(UPARAM(ref) TArray<FLSystemRule>& Rules);

	// ========================================================================
	// Rule Analysis
	// ========================================================================

	/**
	 * Get unique predecessors from a rule set.
	 * @param Rules Array of rules to analyze
	 * @return Array of unique predecessor symbols
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Get Unique Predecessors"))
	static TArray<FString> GetUniquePredecessors(const TArray<FLSystemRule>& Rules);

	/**
	 * Calculate the average growth factor for a rule set.
	 * Growth factor = average(successor.length / predecessor.length * probability)
	 * @param Rules Array of rules to analyze
	 * @return Average growth factor (values > 1 mean string grows, < 1 means shrinks)
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Calculate Growth Factor"))
	static float CalculateGrowthFactor(const TArray<FLSystemRule>& Rules);

	/**
	 * Check if a rule set contains any stochastic rules.
	 * @param Rules Array of rules to check
	 * @return True if any rule has probability != 1.0
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Has Stochastic Rules"))
	static bool HasStochasticRules(const TArray<FLSystemRule>& Rules);

	/**
	 * Check if a rule set contains any context-sensitive rules.
	 * @param Rules Array of rules to check
	 * @return True if any rule has context defined
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Has Context-Sensitive Rules"))
	static bool HasContextSensitiveRules(const TArray<FLSystemRule>& Rules);

	// ========================================================================
	// Utility
	// ========================================================================

	/**
	 * Convert a rule to its string representation.
	 * @param Rule The rule to convert
	 * @return String representation of the rule
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Rule To String"))
	static FString RuleToString(const FLSystemRule& Rule);

	/**
	 * Get a description of the rule type.
	 * @param Rule The rule to describe
	 * @return Description string (e.g., "Simple", "Stochastic", "Context-Sensitive")
	 */
	UFUNCTION(BlueprintPure, Category = "LSystem|Rules",
		meta = (DisplayName = "Get Rule Type Description"))
	static FString GetRuleTypeDescription(const FLSystemRule& Rule);
};
