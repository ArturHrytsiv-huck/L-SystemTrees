// LSystemGenerator.cpp
// Core L-System string generator implementation
// Part of LSystemTrees Plugin - Phase 2

#include "Core/LSystem/LSystemGenerator.h"
#include "Async/Async.h"
#include "HAL/RunnableThread.h"

DEFINE_LOG_CATEGORY(LogLSystem);

// ============================================================================
// FLSystemAsyncTask - Async generation task
// ============================================================================

class FLSystemAsyncTask : public FRunnable
{
public:
	FLSystemAsyncTask(ULSystemGenerator* InGenerator, int32 InIterations)
		: Generator(InGenerator)
		, Iterations(InIterations)
		, bStopping(false)
	{
	}

	virtual ~FLSystemAsyncTask()
	{
	}

	virtual bool Init() override
	{
		return true;
	}

	virtual uint32 Run() override
	{
		if (!Generator.IsValid())
		{
			return 1;
		}

		// Perform generation
		FLSystemGenerationResult Result = Generator->DoGeneration(Iterations, true);

		// Notify completion on game thread
		if (Generator.IsValid())
		{
			AsyncTask(ENamedThreads::GameThread, [this, Result]()
			{
				if (Generator.IsValid())
				{
					Generator->HandleAsyncComplete(Result);
				}
			});
		}

		return 0;
	}

	virtual void Stop() override
	{
		bStopping = true;
	}

	virtual void Exit() override
	{
	}

	bool IsStopping() const
	{
		return bStopping;
	}

private:
	TWeakObjectPtr<ULSystemGenerator> Generator;
	int32 Iterations;
	TAtomic<bool> bStopping;
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

ULSystemGenerator::ULSystemGenerator()
	: bLookupDirty(true)
	, bCancelRequested(false)
	, CurrentAsyncTask(nullptr)
	, CurrentAsyncThread(nullptr)
{
	Config = FLSystemConfig();
	RandomStream.Initialize(FMath::Rand());
}

ULSystemGenerator::~ULSystemGenerator()
{
	// Cancel any running async task
	CancelAsyncGeneration();

	// Wait for thread to finish
	if (CurrentAsyncThread)
	{
		CurrentAsyncThread->WaitForCompletion();
		delete CurrentAsyncThread;
		CurrentAsyncThread = nullptr;
	}

	if (CurrentAsyncTask)
	{
		delete CurrentAsyncTask;
		CurrentAsyncTask = nullptr;
	}
}

// ============================================================================
// Initialization Methods
// ============================================================================

void ULSystemGenerator::Initialize(const FString& Axiom)
{
	FScopeLock Lock(&StateLock);

	CurrentAxiom = Axiom;
	State.Initialize(Axiom);
	Statistics.Reset();

	UE_LOG(LogLSystem, Verbose, TEXT("Initialized with axiom: %s"), *Axiom);
}

void ULSystemGenerator::Reset()
{
	FScopeLock Lock(&StateLock);

	CurrentAxiom.Empty();
	Rules.Empty();
	State.Reset();
	Statistics.Reset();
	RuleLookup.Empty();
	ProbabilityTotals.Empty();
	bLookupDirty = true;
}

// ============================================================================
// Rule Management Methods
// ============================================================================

void ULSystemGenerator::AddRule(const FLSystemRule& Rule)
{
	if (!Rule.IsValid())
	{
		UE_LOG(LogLSystem, Warning, TEXT("Attempted to add invalid rule: %s"), *Rule.ToString());
		return;
	}

	FScopeLock Lock(&StateLock);

	Rules.Add(Rule);
	bLookupDirty = true;

	UE_LOG(LogLSystem, Verbose, TEXT("Added rule: %s"), *Rule.ToString());
}

void ULSystemGenerator::AddRuleSimple(const FString& Predecessor, const FString& Successor)
{
	AddRule(FLSystemRule(Predecessor, Successor, 1.0f));
}

void ULSystemGenerator::AddRuleStochastic(const FString& Predecessor, const FString& Successor, float Probability)
{
	AddRule(FLSystemRule(Predecessor, Successor, Probability));
}

void ULSystemGenerator::AddContextRule(const FString& LeftContext, const FString& Predecessor,
                                        const FString& RightContext, const FString& Successor,
                                        float Probability)
{
	AddRule(FLSystemRule(LeftContext, Predecessor, RightContext, Successor, Probability));
}

bool ULSystemGenerator::RemoveRule(const FString& Predecessor)
{
	FScopeLock Lock(&StateLock);

	int32 RemovedCount = Rules.RemoveAll([&Predecessor](const FLSystemRule& Rule)
	{
		return Rule.Predecessor == Predecessor;
	});

	if (RemovedCount > 0)
	{
		bLookupDirty = true;
		UE_LOG(LogLSystem, Verbose, TEXT("Removed %d rule(s) for predecessor: %s"), RemovedCount, *Predecessor);
	}

	return RemovedCount > 0;
}

bool ULSystemGenerator::RemoveSpecificRule(const FLSystemRule& Rule)
{
	FScopeLock Lock(&StateLock);

	int32 RemovedCount = Rules.RemoveAll([&Rule](const FLSystemRule& ExistingRule)
	{
		return ExistingRule == Rule;
	});

	if (RemovedCount > 0)
	{
		bLookupDirty = true;
	}

	return RemovedCount > 0;
}

void ULSystemGenerator::ClearRules()
{
	FScopeLock Lock(&StateLock);

	Rules.Empty();
	RuleLookup.Empty();
	ProbabilityTotals.Empty();
	bLookupDirty = true;

	UE_LOG(LogLSystem, Verbose, TEXT("Cleared all rules"));
}

int32 ULSystemGenerator::GetRuleCount() const
{
	return Rules.Num();
}

bool ULSystemGenerator::HasRuleForSymbol(const FString& Symbol) const
{
	if (Symbol.Len() != 1)
	{
		return false;
	}

	for (const FLSystemRule& Rule : Rules)
	{
		if (Rule.Predecessor == Symbol)
		{
			return true;
		}
	}
	return false;
}

// ============================================================================
// Generation Methods (Synchronous)
// ============================================================================

FLSystemGenerationResult ULSystemGenerator::Generate(int32 Iterations)
{
	return DoGeneration(Iterations, false);
}

FString ULSystemGenerator::GenerateString(int32 Iterations)
{
	FLSystemGenerationResult Result = Generate(Iterations);
	if (Result.bSuccess)
	{
		return Result.GeneratedString;
	}

	UE_LOG(LogLSystem, Warning, TEXT("Generation failed: %s"), *Result.ErrorMessage);
	return FString();
}

FString ULSystemGenerator::PerformSingleIteration(const FString& InputString)
{
	if (bLookupDirty)
	{
		BuildRuleLookup();
	}
	return ApplyRules(InputString);
}

// ============================================================================
// Generation Methods (Asynchronous)
// ============================================================================

void ULSystemGenerator::GenerateAsync(int32 Iterations)
{
	// Check if already generating
	if (IsGenerating())
	{
		UE_LOG(LogLSystem, Warning, TEXT("Async generation already in progress"));
		return;
	}

	// Validate before starting
	FString ValidationError;
	if (!Validate(ValidationError))
	{
		// Fire completion with error on game thread
		FLSystemGenerationResult FailResult = FLSystemGenerationResult::Failure(ValidationError);
		OnGenerationComplete.Broadcast(FailResult);
		return;
	}

	// Mark as generating
	{
		FScopeLock Lock(&StateLock);
		State.bIsGenerating = true;
		State.ProgressPercent = 0.0f;
	}

	bCancelRequested = false;

	// Clean up previous task if any
	if (CurrentAsyncThread)
	{
		CurrentAsyncThread->WaitForCompletion();
		delete CurrentAsyncThread;
		CurrentAsyncThread = nullptr;
	}
	if (CurrentAsyncTask)
	{
		delete CurrentAsyncTask;
		CurrentAsyncTask = nullptr;
	}

	// Create and start new task
	CurrentAsyncTask = new FLSystemAsyncTask(this, Iterations);
	CurrentAsyncThread = FRunnableThread::Create(CurrentAsyncTask, TEXT("LSystemGenerator"));

	UE_LOG(LogLSystem, Log, TEXT("Started async generation with %d iterations"), Iterations);
}

void ULSystemGenerator::CancelAsyncGeneration()
{
	if (IsGenerating())
	{
		bCancelRequested = true;

		if (CurrentAsyncTask)
		{
			CurrentAsyncTask->Stop();
		}

		UE_LOG(LogLSystem, Log, TEXT("Async generation cancelled"));
	}
}

bool ULSystemGenerator::IsGenerating() const
{
	FScopeLock Lock(&StateLock);
	return State.bIsGenerating;
}

// ============================================================================
// State and Statistics
// ============================================================================

FLSystemState ULSystemGenerator::GetCurrentState() const
{
	FScopeLock Lock(&StateLock);
	return State;
}

FLSystemStatistics ULSystemGenerator::GetStatistics() const
{
	FScopeLock Lock(&StateLock);
	return Statistics;
}

bool ULSystemGenerator::IsValid() const
{
	FString Error;
	return Validate(Error);
}

bool ULSystemGenerator::Validate(FString& OutError) const
{
	if (CurrentAxiom.IsEmpty())
	{
		OutError = TEXT("Axiom cannot be empty");
		return false;
	}

	if (Rules.Num() == 0)
	{
		OutError = TEXT("No rules defined");
		return false;
	}

	// Validate each rule
	for (int32 i = 0; i < Rules.Num(); ++i)
	{
		FString RuleError;
		if (!Rules[i].Validate(RuleError))
		{
			OutError = FString::Printf(TEXT("Invalid rule at index %d: %s"), i, *RuleError);
			return false;
		}
	}

	OutError.Empty();
	return true;
}

// ============================================================================
// Utility Methods
// ============================================================================

void ULSystemGenerator::SetRandomSeed(int32 Seed)
{
	Config.RandomSeed = Seed;
	if (Seed != 0)
	{
		RandomStream.Initialize(Seed);
	}
	else
	{
		RandomStream.Initialize(FMath::Rand());
	}
}

int32 ULSystemGenerator::EstimateStringLength(int32 Iterations) const
{
	if (CurrentAxiom.IsEmpty() || Rules.Num() == 0)
	{
		return 0;
	}

	// Calculate average growth factor
	float TotalGrowth = 0.0f;
	float TotalWeight = 0.0f;

	for (const FLSystemRule& Rule : Rules)
	{
		float Growth = static_cast<float>(Rule.Successor.Len());
		float Weight = Rule.Probability;
		TotalGrowth += Growth * Weight;
		TotalWeight += Weight;
	}

	float AverageGrowth = TotalWeight > 0.0f ? TotalGrowth / TotalWeight : 1.0f;

	// Estimate: axiom_length * growth^iterations
	float Estimate = static_cast<float>(CurrentAxiom.Len()) *
	                 FMath::Pow(FMath::Max(AverageGrowth, 1.0f), static_cast<float>(Iterations));

	return FMath::TruncToInt(FMath::Min(Estimate, static_cast<float>(MAX_int32)));
}

TMap<FString, int32> ULSystemGenerator::CountSymbols(const FString& InputString)
{
	TMap<FString, int32> Counts;

	for (int32 i = 0; i < InputString.Len(); ++i)
	{
		FString Symbol = FString::Chr(InputString[i]);
		int32& Count = Counts.FindOrAdd(Symbol);
		Count++;
	}

	return Counts;
}

// ============================================================================
// Internal Methods
// ============================================================================

FString ULSystemGenerator::ApplyRules(const FString& InputString)
{
	const int32 InputLength = InputString.Len();

	// Pre-allocate result string with estimated capacity
	int32 EstimatedCapacity = InputLength * 2;
	FString Result;
	Result.Reserve(EstimatedCapacity);

	int32 RulesAppliedThisIteration = 0;
	int32 ContextRulesAppliedThisIteration = 0;

	// Process each character
	for (int32 i = 0; i < InputLength; ++i)
	{
		TCHAR CurrentChar = InputString[i];

		// Get context characters
		TCHAR LeftContext = (i > 0) ? InputString[i - 1] : TEXT('\0');
		TCHAR RightContext = (i < InputLength - 1) ? InputString[i + 1] : TEXT('\0');

		// Try to find a matching rule
		const FLSystemRule* SelectedRule = SelectRule(CurrentChar, LeftContext, RightContext);

		if (SelectedRule)
		{
			// Apply the rule
			Result.Append(SelectedRule->Successor);
			RulesAppliedThisIteration++;

			if (SelectedRule->IsContextSensitive())
			{
				ContextRulesAppliedThisIteration++;
			}
		}
		else
		{
			// No rule found - keep the character unchanged (identity rule)
			Result.AppendChar(CurrentChar);
		}

		// Check length limit during iteration
		if (Result.Len() > Config.MaxStringLength)
		{
			UE_LOG(LogLSystem, Warning, TEXT("String length exceeded maximum during iteration. Truncating."));
			Result.LeftInline(Config.MaxStringLength);
			break;
		}
	}

	// Update statistics
	{
		FScopeLock Lock(&StateLock);
		Statistics.RulesApplied += RulesAppliedThisIteration;
		Statistics.ContextRulesApplied += ContextRulesAppliedThisIteration;
	}

	return Result;
}

const FLSystemRule* ULSystemGenerator::SelectRule(TCHAR Symbol, TCHAR LeftContext, TCHAR RightContext)
{
	// Find rules for this symbol
	const TArray<const FLSystemRule*>* RulesPtr = RuleLookup.Find(Symbol);

	if (!RulesPtr || RulesPtr->Num() == 0)
	{
		return nullptr;
	}

	const TArray<const FLSystemRule*>& MatchingRules = *RulesPtr;

	// Filter rules by context and collect matching ones
	TArray<const FLSystemRule*> ContextMatchingRules;
	int32 MaxSpecificity = -1;

	for (const FLSystemRule* Rule : MatchingRules)
	{
		if (Rule->MatchesContext(LeftContext, RightContext))
		{
			int32 Specificity = Rule->GetContextSpecificity();
			if (Specificity > MaxSpecificity)
			{
				// Found more specific rule, reset array
				ContextMatchingRules.Empty();
				MaxSpecificity = Specificity;
			}

			if (Specificity == MaxSpecificity)
			{
				ContextMatchingRules.Add(Rule);
			}
		}
	}

	if (ContextMatchingRules.Num() == 0)
	{
		return nullptr;
	}

	// If only one matching rule, return it
	if (ContextMatchingRules.Num() == 1)
	{
		return ContextMatchingRules[0];
	}

	// Stochastic selection among equally specific rules
	float TotalProbability = 0.0f;
	for (const FLSystemRule* Rule : ContextMatchingRules)
	{
		TotalProbability += Rule->Probability;
	}

	if (TotalProbability <= 0.0f)
	{
		// Fallback: uniform selection
		int32 RandomIndex = RandomStream.RandRange(0, ContextMatchingRules.Num() - 1);
		return ContextMatchingRules[RandomIndex];
	}

	// Generate random value in [0, TotalProbability)
	float RandomValue = RandomStream.FRandRange(0.0f, TotalProbability);

	// Select rule based on cumulative probability
	float CumulativeProbability = 0.0f;
	for (const FLSystemRule* Rule : ContextMatchingRules)
	{
		CumulativeProbability += Rule->Probability;
		if (RandomValue < CumulativeProbability)
		{
			return Rule;
		}
	}

	// Fallback: return last rule
	return ContextMatchingRules.Last();
}

void ULSystemGenerator::BuildRuleLookup()
{
	FScopeLock Lock(&StateLock);

	RuleLookup.Empty();
	ProbabilityTotals.Empty();

	for (const FLSystemRule& Rule : Rules)
	{
		if (Rule.IsValid())
		{
			TCHAR PredecessorChar = Rule.GetPredecessorChar();

			// Add to lookup
			TArray<const FLSystemRule*>& RulesForSymbol = RuleLookup.FindOrAdd(PredecessorChar);
			RulesForSymbol.Add(&Rule);

			// Accumulate probability
			float& TotalProb = ProbabilityTotals.FindOrAdd(PredecessorChar);
			TotalProb += Rule.Probability;
		}
	}

	// Sort each rule array by specificity (more specific first)
	for (auto& Pair : RuleLookup)
	{
		Pair.Value.Sort([](const FLSystemRule& A, const FLSystemRule& B)
		{
			return A.GetContextSpecificity() > B.GetContextSpecificity();
		});
	}

	bLookupDirty = false;

	UE_LOG(LogLSystem, Verbose, TEXT("Built rule lookup with %d unique predecessors"), RuleLookup.Num());
}

bool ULSystemGenerator::CheckTermination(const FString& CurrentString, int32 Iteration, FString& OutReason) const
{
	// Check iteration limit
	if (Iteration >= Config.MaxIterations)
	{
		OutReason = FString::Printf(TEXT("Reached maximum iterations (%d)"), Config.MaxIterations);
		return true;
	}

	// Check string length limit
	if (CurrentString.Len() >= Config.MaxStringLength)
	{
		OutReason = FString::Printf(TEXT("Reached maximum string length (%d)"), Config.MaxStringLength);
		return true;
	}

	// Check for cancellation (async only)
	if (bCancelRequested)
	{
		OutReason = TEXT("Generation was cancelled");
		return true;
	}

	return false;
}

void ULSystemGenerator::UpdateStatistics(const FString& FinalString, int32 Iterations, double StartTime)
{
	FScopeLock Lock(&StateLock);

	const double EndTime = FPlatformTime::Seconds();

	Statistics.TotalIterations = Iterations;
	Statistics.FinalStringLength = FinalString.Len();
	Statistics.GenerationTimeMs = static_cast<float>((EndTime - StartTime) * 1000.0);

	// Calculate symbol counts
	CalculateSymbolCounts(FinalString);

	UE_LOG(LogLSystem, Log, TEXT("Generation complete: %s"), *Statistics.ToString());
}

void ULSystemGenerator::CalculateSymbolCounts(const FString& InputString)
{
	Statistics.SymbolCounts.Empty();

	for (int32 i = 0; i < InputString.Len(); ++i)
	{
		FString Symbol = FString::Chr(InputString[i]);
		int32& Count = Statistics.SymbolCounts.FindOrAdd(Symbol);
		Count++;
	}
}

void ULSystemGenerator::LogIteration(int32 Iteration, const FString& CurrentString)
{
	if (Config.bEnableDetailedLogging)
	{
		// Truncate string for logging if too long
		FString LogString = CurrentString;
		if (LogString.Len() > 200)
		{
			LogString = LogString.Left(100) + TEXT("...") + LogString.Right(97);
		}

		UE_LOG(LogLSystem, Log, TEXT("Iteration %d: Length=%d, String=%s"),
		       Iteration, CurrentString.Len(), *LogString);
	}
}

FLSystemGenerationResult ULSystemGenerator::DoGeneration(int32 Iterations, bool bAsync)
{
	// Validate configuration
	FString ValidationError;
	if (!Validate(ValidationError))
	{
		return FLSystemGenerationResult::Failure(ValidationError);
	}

	// Clamp iterations
	Iterations = FMath::Clamp(Iterations, 1, Config.MaxIterations);

	// Initialize random stream
	if (Config.RandomSeed != 0)
	{
		RandomStream.Initialize(Config.RandomSeed);
	}
	else
	{
		RandomStream.Initialize(FMath::Rand());
	}

	// Build rule lookup if needed
	if (bLookupDirty)
	{
		BuildRuleLookup();
	}

	// Start generation
	const double StartTime = FPlatformTime::Seconds();

	{
		FScopeLock Lock(&StateLock);
		State.bIsGenerating = true;
		State.CurrentString = CurrentAxiom;
		State.CurrentIteration = 0;
		State.History.Empty();
		State.ProgressPercent = 0.0f;
		Statistics.Reset();
	}

	FString CurrentString = CurrentAxiom;
	TArray<FString> History;

	if (Config.bStoreHistory)
	{
		History.Add(CurrentString);
	}

	// Log initial state
	LogIteration(0, CurrentString);

	FString TerminationReason;

	// Main generation loop
	for (int32 i = 0; i < Iterations; ++i)
	{
		// Check termination conditions
		if (CheckTermination(CurrentString, i, TerminationReason))
		{
			UE_LOG(LogLSystem, Log, TEXT("Generation terminated early: %s"), *TerminationReason);
			break;
		}

		// Store previous string for change detection
		FString PreviousString = CurrentString;

		// Apply rules
		CurrentString = ApplyRules(CurrentString);

		// Update state
		{
			FScopeLock Lock(&StateLock);
			State.CurrentString = CurrentString;
			State.CurrentIteration = i + 1;
			State.ProgressPercent = static_cast<float>(i + 1) / static_cast<float>(Iterations);

			if (Config.bStoreHistory)
			{
				State.History.Add(CurrentString);
			}
		}

		// Store in local history
		if (Config.bStoreHistory)
		{
			History.Add(CurrentString);
		}

		// Log iteration
		LogIteration(i + 1, CurrentString);

		// Fire progress delegate (on game thread if async)
		if (bAsync && OnGenerationProgress.IsBound())
		{
			float Progress = static_cast<float>(i + 1) / static_cast<float>(Iterations);
			AsyncTask(ENamedThreads::GameThread, [this, Progress]()
			{
				if (IsValid())
				{
					OnGenerationProgress.Broadcast(Progress);
				}
			});
		}

		// Fire iteration complete delegate
		if (OnIterationComplete.IsBound())
		{
			int32 IterNum = i + 1;
			FString IterString = CurrentString;
			if (bAsync)
			{
				AsyncTask(ENamedThreads::GameThread, [this, IterNum, IterString]()
				{
					if (IsValid())
					{
						OnIterationComplete.Broadcast(IterNum, IterString);
					}
				});
			}
			else
			{
				OnIterationComplete.Broadcast(IterNum, IterString);
			}
		}

		// Check if string changed (detect potential infinite loops with no effect)
		if (PreviousString == CurrentString)
		{
			UE_LOG(LogLSystem, Log, TEXT("String unchanged at iteration %d, stopping"), i + 1);
			break;
		}
	}

	// Mark generation complete
	{
		FScopeLock Lock(&StateLock);
		State.bIsGenerating = false;
		State.ProgressPercent = 1.0f;
	}

	// Update statistics
	int32 ActualIterations;
	{
		FScopeLock Lock(&StateLock);
		ActualIterations = State.CurrentIteration;
	}
	UpdateStatistics(CurrentString, ActualIterations, StartTime);

	// Handle cancellation
	if (bCancelRequested)
	{
		return FLSystemGenerationResult::Cancelled();
	}

	// Copy statistics
	FLSystemStatistics FinalStats;
	{
		FScopeLock Lock(&StateLock);
		FinalStats = Statistics;
	}

	return FLSystemGenerationResult::Success(CurrentString, History, FinalStats);
}

void ULSystemGenerator::HandleAsyncComplete(const FLSystemGenerationResult& Result)
{
	{
		FScopeLock Lock(&StateLock);
		State.bIsGenerating = false;
		State.ProgressPercent = 1.0f;
	}

	// Fire completion delegate
	OnGenerationComplete.Broadcast(Result);

	UE_LOG(LogLSystem, Log, TEXT("Async generation complete. Success=%s"), Result.bSuccess ? TEXT("true") : TEXT("false"));
}
