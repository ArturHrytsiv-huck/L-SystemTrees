// LSystemRule.cpp
// Blueprint function library for L-System rule operations
// Part of LSystemTrees Plugin - Phase 2

#include "Core/LSystem/LSystemRule.h"

// ============================================================================
// Rule Creation
// ============================================================================

FLSystemRule ULSystemRuleLibrary::MakeSimpleRule(const FString& Predecessor, const FString& Successor)
{
	return FLSystemRule(Predecessor, Successor, 1.0f);
}

FLSystemRule ULSystemRuleLibrary::MakeStochasticRule(const FString& Predecessor,
                                                      const FString& Successor,
                                                      float Probability)
{
	return FLSystemRule(Predecessor, Successor, Probability);
}

FLSystemRule ULSystemRuleLibrary::MakeContextRule(const FString& LeftContext,
                                                   const FString& Predecessor,
                                                   const FString& RightContext,
                                                   const FString& Successor,
                                                   float Probability)
{
	return FLSystemRule(LeftContext, Predecessor, RightContext, Successor, Probability);
}

bool ULSystemRuleLibrary::ParseRuleString(const FString& RuleString,
                                          FLSystemRule& OutRule,
                                          FString& OutError)
{
	// Trim whitespace
	FString Input = RuleString.TrimStartAndEnd();

	if (Input.IsEmpty())
	{
		OutError = TEXT("Rule string is empty");
		return false;
	}

	// Initialize output
	OutRule = FLSystemRule();
	float Probability = 1.0f;

	// Check for probability at the end: "... (0.5)"
	int32 ProbStart = Input.Find(TEXT("("), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	int32 ProbEnd = Input.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	if (ProbStart != INDEX_NONE && ProbEnd != INDEX_NONE && ProbEnd > ProbStart)
	{
		FString ProbStr = Input.Mid(ProbStart + 1, ProbEnd - ProbStart - 1).TrimStartAndEnd();
		if (ProbStr.IsNumeric() || ProbStr.Contains(TEXT(".")))
		{
			Probability = FCString::Atof(*ProbStr);
			Input = Input.Left(ProbStart).TrimEnd();
		}
	}

	// Find the arrow "->"
	int32 ArrowPos = Input.Find(TEXT("->"));
	if (ArrowPos == INDEX_NONE)
	{
		// Try alternative arrow format "→"
		ArrowPos = Input.Find(TEXT("→"));
		if (ArrowPos == INDEX_NONE)
		{
			OutError = TEXT("Rule must contain '->' or '→' separator");
			return false;
		}
	}

	// Split into left side (predecessor with optional context) and right side (successor)
	FString LeftSide = Input.Left(ArrowPos).TrimStartAndEnd();
	FString RightSide = Input.Mid(ArrowPos + 2).TrimStartAndEnd();

	if (RightSide.IsEmpty())
	{
		OutError = TEXT("Successor (right side of ->) cannot be empty");
		return false;
	}

	// Parse left side for context: "A < B > C" or "A < B" or "B > C" or "B"
	FString LeftContext;
	FString Predecessor;
	FString RightContext;

	// Check for left context: "X < Y"
	int32 LeftArrowPos = LeftSide.Find(TEXT("<"));
	// Check for right context: "Y > Z"
	int32 RightArrowPos = LeftSide.Find(TEXT(">"));

	if (LeftArrowPos != INDEX_NONE && RightArrowPos != INDEX_NONE)
	{
		// Full context: "A < B > C"
		LeftContext = LeftSide.Left(LeftArrowPos).TrimStartAndEnd();
		Predecessor = LeftSide.Mid(LeftArrowPos + 1, RightArrowPos - LeftArrowPos - 1).TrimStartAndEnd();
		RightContext = LeftSide.Mid(RightArrowPos + 1).TrimStartAndEnd();
	}
	else if (LeftArrowPos != INDEX_NONE)
	{
		// Left context only: "A < B"
		LeftContext = LeftSide.Left(LeftArrowPos).TrimStartAndEnd();
		Predecessor = LeftSide.Mid(LeftArrowPos + 1).TrimStartAndEnd();
	}
	else if (RightArrowPos != INDEX_NONE)
	{
		// Right context only: "B > C"
		Predecessor = LeftSide.Left(RightArrowPos).TrimStartAndEnd();
		RightContext = LeftSide.Mid(RightArrowPos + 1).TrimStartAndEnd();
	}
	else
	{
		// No context: "B"
		Predecessor = LeftSide;
	}

	// Validate predecessor
	if (Predecessor.Len() != 1)
	{
		OutError = FString::Printf(TEXT("Predecessor must be exactly 1 character, got '%s'"), *Predecessor);
		return false;
	}

	// Validate contexts (must be 0 or 1 character)
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

	// Build the rule
	OutRule.LeftContext = LeftContext;
	OutRule.Predecessor = Predecessor;
	OutRule.RightContext = RightContext;
	OutRule.Successor = RightSide;
	OutRule.Probability = FMath::Clamp(Probability, 0.0f, 1.0f);

	OutError.Empty();
	return true;
}

// ============================================================================
// Rule Validation
// ============================================================================

bool ULSystemRuleLibrary::ValidateRule(const FLSystemRule& Rule, FString& OutError)
{
	return Rule.Validate(OutError);
}

bool ULSystemRuleLibrary::IsRuleValid(const FLSystemRule& Rule)
{
	return Rule.IsValid();
}

bool ULSystemRuleLibrary::ValidateRulesArray(const TArray<FLSystemRule>& Rules,
                                             TArray<FString>& OutErrors)
{
	OutErrors.Empty();
	OutErrors.SetNum(Rules.Num());

	bool bAllValid = true;

	for (int32 i = 0; i < Rules.Num(); ++i)
	{
		FString Error;
		if (!Rules[i].Validate(Error))
		{
			OutErrors[i] = FString::Printf(TEXT("Rule %d: %s"), i, *Error);
			bAllValid = false;
		}
	}

	return bAllValid;
}

// ============================================================================
// Rule Manipulation
// ============================================================================

void ULSystemRuleLibrary::NormalizeProbabilities(TArray<FLSystemRule>& Rules)
{
	// Group rules by predecessor (including context for proper grouping)
	TMap<FString, TArray<int32>> PredecessorGroups;

	for (int32 i = 0; i < Rules.Num(); ++i)
	{
		// Key includes predecessor only (context doesn't affect probability grouping)
		const FString& Key = Rules[i].Predecessor;
		PredecessorGroups.FindOrAdd(Key).Add(i);
	}

	// Normalize each group
	for (auto& Pair : PredecessorGroups)
	{
		const TArray<int32>& Indices = Pair.Value;

		if (Indices.Num() <= 1)
		{
			continue; // Single rule doesn't need normalization
		}

		// Calculate total probability for this group
		float TotalProbability = 0.0f;
		for (int32 Index : Indices)
		{
			TotalProbability += Rules[Index].Probability;
		}

		// Normalize if total is not 1.0 and is > 0
		if (TotalProbability > 0.0f && !FMath::IsNearlyEqual(TotalProbability, 1.0f))
		{
			for (int32 Index : Indices)
			{
				Rules[Index].Probability /= TotalProbability;
			}
		}
	}
}

TArray<FLSystemRule> ULSystemRuleLibrary::GetRulesForPredecessor(const TArray<FLSystemRule>& Rules,
                                                                  const FString& Predecessor)
{
	TArray<FLSystemRule> Result;

	for (const FLSystemRule& Rule : Rules)
	{
		if (Rule.Predecessor == Predecessor)
		{
			Result.Add(Rule);
		}
	}

	return Result;
}

TArray<FLSystemRule> ULSystemRuleLibrary::GetContextSensitiveRules(const TArray<FLSystemRule>& Rules)
{
	TArray<FLSystemRule> Result;

	for (const FLSystemRule& Rule : Rules)
	{
		if (Rule.IsContextSensitive())
		{
			Result.Add(Rule);
		}
	}

	return Result;
}

TArray<FLSystemRule> ULSystemRuleLibrary::GetContextFreeRules(const TArray<FLSystemRule>& Rules)
{
	TArray<FLSystemRule> Result;

	for (const FLSystemRule& Rule : Rules)
	{
		if (!Rule.IsContextSensitive())
		{
			Result.Add(Rule);
		}
	}

	return Result;
}

void ULSystemRuleLibrary::SortRulesBySpecificity(TArray<FLSystemRule>& Rules)
{
	// Sort by context specificity (descending) - more specific rules first
	Rules.Sort([](const FLSystemRule& A, const FLSystemRule& B)
	{
		return A.GetContextSpecificity() > B.GetContextSpecificity();
	});
}

// ============================================================================
// Rule Analysis
// ============================================================================

TArray<FString> ULSystemRuleLibrary::GetUniquePredecessors(const TArray<FLSystemRule>& Rules)
{
	TSet<FString> UniqueSet;

	for (const FLSystemRule& Rule : Rules)
	{
		UniqueSet.Add(Rule.Predecessor);
	}

	return UniqueSet.Array();
}

float ULSystemRuleLibrary::CalculateGrowthFactor(const TArray<FLSystemRule>& Rules)
{
	if (Rules.Num() == 0)
	{
		return 1.0f;
	}

	float TotalGrowth = 0.0f;
	float TotalWeight = 0.0f;

	for (const FLSystemRule& Rule : Rules)
	{
		// Growth = successor length / predecessor length (predecessor is always 1)
		float Growth = static_cast<float>(Rule.Successor.Len());
		float Weight = Rule.Probability;

		TotalGrowth += Growth * Weight;
		TotalWeight += Weight;
	}

	if (TotalWeight > 0.0f)
	{
		return TotalGrowth / TotalWeight;
	}

	return 1.0f;
}

bool ULSystemRuleLibrary::HasStochasticRules(const TArray<FLSystemRule>& Rules)
{
	for (const FLSystemRule& Rule : Rules)
	{
		if (!FMath::IsNearlyEqual(Rule.Probability, 1.0f))
		{
			return true;
		}
	}

	return false;
}

bool ULSystemRuleLibrary::HasContextSensitiveRules(const TArray<FLSystemRule>& Rules)
{
	for (const FLSystemRule& Rule : Rules)
	{
		if (Rule.IsContextSensitive())
		{
			return true;
		}
	}

	return false;
}

// ============================================================================
// Utility
// ============================================================================

FString ULSystemRuleLibrary::RuleToString(const FLSystemRule& Rule)
{
	return Rule.ToString();
}

FString ULSystemRuleLibrary::GetRuleTypeDescription(const FLSystemRule& Rule)
{
	TArray<FString> Types;

	if (Rule.IsContextSensitive())
	{
		if (Rule.LeftContext.Len() > 0 && Rule.RightContext.Len() > 0)
		{
			Types.Add(TEXT("Full Context-Sensitive"));
		}
		else if (Rule.LeftContext.Len() > 0)
		{
			Types.Add(TEXT("Left Context-Sensitive"));
		}
		else
		{
			Types.Add(TEXT("Right Context-Sensitive"));
		}
	}
	else
	{
		Types.Add(TEXT("Context-Free"));
	}

	if (!FMath::IsNearlyEqual(Rule.Probability, 1.0f))
	{
		Types.Add(TEXT("Stochastic"));
	}
	else
	{
		Types.Add(TEXT("Deterministic"));
	}

	return FString::Join(Types, TEXT(", "));
}
