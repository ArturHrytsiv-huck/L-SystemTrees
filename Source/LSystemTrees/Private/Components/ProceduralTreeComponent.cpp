// ProceduralTreeComponent.cpp
// Main component for procedural tree generation using L-Systems
// Part of LSystemTrees Plugin - Phase 3

#include "Components/ProceduralTreeComponent.h"
#include "Core/LSystem/LSystemGenerator.h"
#include "Core/TreeGeometry/TurtleInterpreter.h"
#include "Core/TreeGeometry/TreeGeometry.h"
#include "Core/Utilities/DebugDraw.h"

// ============================================================================
// Constructor
// ============================================================================

UProceduralTreeComponent::UProceduralTreeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Axiom(TEXT("F"))
	, Iterations(4)
	, RandomSeed(0)
	, bGenerateOnStart(false)
	, BarkMaterial(nullptr)
	, LeafMaterial(nullptr)
	, Generator(nullptr)
	, Interpreter(nullptr)
	, GeometryBuilder(nullptr)
	, CurrentLODIndex(0)
{
	// Set up default 3D tree rule with pitch variations for depth
	// Uses ^ (pitch up) and & (pitch down) for 3D growth
	// Uses / and \ (roll) for branch rotation variety
	FLSystemRule DefaultRule;
	DefaultRule.Predecessor = TEXT("F");
	DefaultRule.Successor = TEXT("FF&[-/F+F+FL]^[+\\F-F-FL]");
	DefaultRule.Probability = 1.0f;
	Rules.Add(DefaultRule);

	// Initialize default LOD levels
	InitializeDefaultLODs();
}

// ============================================================================
// UActorComponent Interface
// ============================================================================

void UProceduralTreeComponent::OnComponentCreated()
{
	Super::OnComponentCreated();
	InitializeGenerators();
}

void UProceduralTreeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnStart)
	{
		GenerateTree();
	}
}

#if WITH_EDITOR
void UProceduralTreeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Auto-regenerate tree when properties change in editor
	if (PropertyChangedEvent.Property != nullptr)
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();

		// List of properties that should trigger regeneration
		static const TSet<FName> RegenerateProperties = {
			GET_MEMBER_NAME_CHECKED(UProceduralTreeComponent, Axiom),
			GET_MEMBER_NAME_CHECKED(UProceduralTreeComponent, Rules),
			GET_MEMBER_NAME_CHECKED(UProceduralTreeComponent, Iterations),
			GET_MEMBER_NAME_CHECKED(UProceduralTreeComponent, RandomSeed),
			GET_MEMBER_NAME_CHECKED(UProceduralTreeComponent, TurtleConfig),
			GET_MEMBER_NAME_CHECKED(UProceduralTreeComponent, GeometryConfig),
			GET_MEMBER_NAME_CHECKED(UProceduralTreeComponent, LODLevels)
		};

		// Regenerate if relevant property changed
		if (RegenerateProperties.Contains(PropertyName))
		{
			GenerateTree();
		}

		// Apply materials if material properties changed
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UProceduralTreeComponent, BarkMaterial) ||
		    PropertyName == GET_MEMBER_NAME_CHECKED(UProceduralTreeComponent, LeafMaterial))
		{
			ApplyMaterials();
		}
	}
}
#endif

// ============================================================================
// Generation Methods
// ============================================================================

void UProceduralTreeComponent::GenerateTree()
{
	// Ensure generators are initialized
	InitializeGenerators();

	if (!Generator || !Interpreter || !GeometryBuilder)
	{
		UE_LOG(LogTemp, Error, TEXT("ProceduralTreeComponent: Failed to initialize generators"));
		OnTreeGenerated.Broadcast(false);
		return;
	}

	// Clear previous data
	ClearTree();
	CachedSegments.Empty();
	CachedLeaves.Empty();
	CachedLODs.Empty();

	// Report progress: Step 1 - L-System Generation
	OnGenerationProgress.Broadcast(1, 4);

	// Step 1: Generate L-System string
	Generator->Reset();
	Generator->Initialize(Axiom);

	// Add all rules
	for (const FLSystemRule& Rule : Rules)
	{
		Generator->AddRule(Rule);
	}

	// Set random seed
	Generator->SetRandomSeed(RandomSeed);

	// Generate the string
	FLSystemGenerationResult GenResult = Generator->Generate(Iterations);
	if (!GenResult.bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("ProceduralTreeComponent: L-System generation failed: %s"), *GenResult.ErrorMessage);
		OnTreeGenerated.Broadcast(false);
		return;
	}

	CachedLSystemString = GenResult.GeneratedString;

	UE_LOG(LogTemp, Log, TEXT("ProceduralTreeComponent: Generated L-System string with %d characters"), CachedLSystemString.Len());

	// Report progress: Step 2 - Turtle Interpretation
	OnGenerationProgress.Broadcast(2, 4);

	// Step 2: Interpret string with turtle
	FTurtleConfig InterpretConfig = TurtleConfig;
	InterpretConfig.RandomSeed = RandomSeed;
	InterpretConfig.LeafSize = GeometryConfig.LeafSize;

	Interpreter->InterpretString(CachedLSystemString, InterpretConfig, CachedSegments, CachedLeaves);

	UE_LOG(LogTemp, Log, TEXT("ProceduralTreeComponent: Created %d segments and %d leaves"),
	       CachedSegments.Num(), CachedLeaves.Num());

	// Report progress: Step 3 - Geometry Generation
	OnGenerationProgress.Broadcast(3, 4);

	// Step 3: Generate mesh geometry for all LOD levels
	if (LODLevels.Num() == 0)
	{
		InitializeDefaultLODs();
	}

	GeometryBuilder->BarkUVTiling = GeometryConfig.BarkUVTiling;
	GeometryBuilder->DefaultLeafSize = GeometryConfig.LeafSize;

	CachedLODs = GeometryBuilder->GenerateMeshLODs(CachedSegments, CachedLeaves, LODLevels);

	if (CachedLODs.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("ProceduralTreeComponent: Failed to generate mesh LODs"));
		OnTreeGenerated.Broadcast(false);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("ProceduralTreeComponent: Generated %d LOD levels"), CachedLODs.Num());

	// Report progress: Step 4 - Apply Mesh
	OnGenerationProgress.Broadcast(4, 4);

	// Step 4: Apply the highest detail LOD to the mesh
	CurrentLODIndex = 0;
	ApplyMeshData(CachedLODs[CurrentLODIndex]);
	ApplyMaterials();

	// Broadcast completion
	OnTreeGenerated.Broadcast(true);
}

void UProceduralTreeComponent::RegenerateWithSeed(int32 Seed)
{
	RandomSeed = Seed;
	GenerateTree();
}

void UProceduralTreeComponent::ClearTree()
{
	ClearAllMeshSections();
	CurrentLODIndex = 0;
}

// ============================================================================
// LOD Control
// ============================================================================

void UProceduralTreeComponent::SetLODLevel(int32 LODIndex)
{
	if (CachedLODs.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProceduralTreeComponent: No LODs available. Generate tree first."));
		return;
	}

	const int32 ClampedIndex = FMath::Clamp(LODIndex, 0, CachedLODs.Num() - 1);

	if (ClampedIndex != CurrentLODIndex)
	{
		CurrentLODIndex = ClampedIndex;
		ApplyMeshData(CachedLODs[CurrentLODIndex]);
		ApplyMaterials();

		UE_LOG(LogTemp, Verbose, TEXT("ProceduralTreeComponent: Switched to LOD %d"), CurrentLODIndex);
	}
}

int32 UProceduralTreeComponent::GetCurrentLODLevel() const
{
	return CurrentLODIndex;
}

int32 UProceduralTreeComponent::GetLODCount() const
{
	return CachedLODs.Num();
}

// ============================================================================
// Statistics
// ============================================================================

FString UProceduralTreeComponent::GetLSystemString() const
{
	return CachedLSystemString;
}

int32 UProceduralTreeComponent::GetBranchSegmentCount() const
{
	return CachedSegments.Num();
}

int32 UProceduralTreeComponent::GetLeafCount() const
{
	return CachedLeaves.Num();
}

int32 UProceduralTreeComponent::GetVertexCount() const
{
	if (CurrentLODIndex >= 0 && CurrentLODIndex < CachedLODs.Num())
	{
		return CachedLODs[CurrentLODIndex].GetVertexCount();
	}
	return 0;
}

int32 UProceduralTreeComponent::GetTriangleCount() const
{
	if (CurrentLODIndex >= 0 && CurrentLODIndex < CachedLODs.Num())
	{
		return CachedLODs[CurrentLODIndex].GetTriangleCount();
	}
	return 0;
}

// ============================================================================
// Debug
// ============================================================================

void UProceduralTreeComponent::DrawDebug(float Duration)
{
#if !UE_BUILD_SHIPPING
	if (CachedSegments.Num() > 0)
	{
		UTreeDebugDraw::DrawBranchSegments(this, CachedSegments, Duration, true);
	}

	if (CachedLeaves.Num() > 0)
	{
		UTreeDebugDraw::DrawLeaves(this, CachedLeaves, Duration);
	}

	// Print string stats
	UTreeDebugDraw::PrintLSystemString(CachedLSystemString, 500);
#endif
}

// ============================================================================
// Internal Methods
// ============================================================================

void UProceduralTreeComponent::InitializeDefaultLODs()
{
	LODLevels.Empty();

	// LOD 0: High detail
	FTreeLODLevel LOD0;
	LOD0.RadialSegments = 12;
	LOD0.ScreenSize = 1.0f;
	LOD0.bIncludeLeaves = true;
	LODLevels.Add(LOD0);

	// LOD 1: Medium detail
	FTreeLODLevel LOD1;
	LOD1.RadialSegments = 8;
	LOD1.ScreenSize = 0.5f;
	LOD1.bIncludeLeaves = true;
	LODLevels.Add(LOD1);

	// LOD 2: Low detail
	FTreeLODLevel LOD2;
	LOD2.RadialSegments = 4;
	LOD2.ScreenSize = 0.25f;
	LOD2.bIncludeLeaves = false;
	LODLevels.Add(LOD2);
}

void UProceduralTreeComponent::InitializeGenerators()
{
	if (!Generator)
	{
		Generator = NewObject<ULSystemGenerator>(this, TEXT("LSystemGenerator"));
	}

	if (!Interpreter)
	{
		Interpreter = NewObject<UTurtleInterpreter>(this, TEXT("TurtleInterpreter"));
	}

	if (!GeometryBuilder)
	{
		GeometryBuilder = NewObject<UTreeGeometry>(this, TEXT("TreeGeometry"));
	}
}

void UProceduralTreeComponent::ApplyMeshData(const FTreeMeshData& MeshData)
{
	// Clear existing mesh
	ClearAllMeshSections();

	if (MeshData.Vertices.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProceduralTreeComponent: No vertices to apply"));
		return;
	}

	// Convert vertex colors to color array
	TArray<FColor> Colors;
	Colors.Reserve(MeshData.VertexColors.Num());
	for (const FLinearColor& LinearColor : MeshData.VertexColors)
	{
		Colors.Add(LinearColor.ToFColor(true));
	}

	// Create Section 0: Branches
	// We use all vertices up to BranchVertexCount for branches
	if (MeshData.BranchVertexCount > 0 && MeshData.BranchTriangleCount > 0)
	{
		TArray<FVector> BranchVertices;
		TArray<FVector> BranchNormals;
		TArray<FVector2D> BranchUVs;
		TArray<FColor> BranchColors;
		TArray<int32> BranchTriangles;
		TArray<FProcMeshTangent> BranchTangents;

		BranchVertices.Reserve(MeshData.BranchVertexCount);
		BranchNormals.Reserve(MeshData.BranchVertexCount);
		BranchUVs.Reserve(MeshData.BranchVertexCount);
		BranchColors.Reserve(MeshData.BranchVertexCount);
		BranchTriangles.Reserve(MeshData.BranchTriangleCount * 3);
		BranchTangents.Reserve(MeshData.BranchVertexCount);

		for (int32 i = 0; i < MeshData.BranchVertexCount; ++i)
		{
			BranchVertices.Add(MeshData.Vertices[i]);
			BranchNormals.Add(MeshData.Normals[i]);
			BranchUVs.Add(MeshData.UVs[i]);
			BranchColors.Add(Colors[i]);
			if (i < MeshData.Tangents.Num())
			{
				BranchTangents.Add(FProcMeshTangent(MeshData.Tangents[i], false));
			}
		}

		for (int32 i = 0; i < MeshData.BranchTriangleCount * 3; ++i)
		{
			BranchTriangles.Add(MeshData.Triangles[i]);
		}

		CreateMeshSection_LinearColor(0, BranchVertices, BranchTriangles, BranchNormals,
		                               BranchUVs, TArray<FLinearColor>(), BranchTangents, true);

		UE_LOG(LogTemp, Verbose, TEXT("ProceduralTreeComponent: Created branch mesh section with %d verts, %d tris"),
		       BranchVertices.Num(), BranchTriangles.Num() / 3);
	}

	// Create Section 1: Leaves
	const int32 LeafVertexStart = MeshData.BranchVertexCount;
	const int32 LeafTriangleStart = MeshData.BranchTriangleCount * 3;
	const int32 LeafVertexCount = MeshData.Vertices.Num() - LeafVertexStart;
	const int32 LeafTriangleCount = (MeshData.Triangles.Num() - LeafTriangleStart) / 3;

	if (LeafVertexCount > 0 && LeafTriangleCount > 0)
	{
		TArray<FVector> LeafVertices;
		TArray<FVector> LeafNormals;
		TArray<FVector2D> LeafUVs;
		TArray<FLinearColor> LeafColors;
		TArray<int32> LeafTriangles;
		TArray<FProcMeshTangent> LeafTangents;

		LeafVertices.Reserve(LeafVertexCount);
		LeafNormals.Reserve(LeafVertexCount);
		LeafUVs.Reserve(LeafVertexCount);
		LeafColors.Reserve(LeafVertexCount);
		LeafTriangles.Reserve(LeafTriangleCount * 3);
		LeafTangents.Reserve(LeafVertexCount);

		for (int32 i = LeafVertexStart; i < MeshData.Vertices.Num(); ++i)
		{
			LeafVertices.Add(MeshData.Vertices[i]);
			LeafNormals.Add(MeshData.Normals[i]);
			LeafUVs.Add(MeshData.UVs[i]);
			LeafColors.Add(MeshData.VertexColors[i]);
			if (i < MeshData.Tangents.Num())
			{
				LeafTangents.Add(FProcMeshTangent(MeshData.Tangents[i], false));
			}
		}

		// Remap triangle indices to local leaf vertex indices
		for (int32 i = LeafTriangleStart; i < MeshData.Triangles.Num(); ++i)
		{
			LeafTriangles.Add(MeshData.Triangles[i] - LeafVertexStart);
		}

		CreateMeshSection_LinearColor(1, LeafVertices, LeafTriangles, LeafNormals,
		                               LeafUVs, LeafColors, LeafTangents, true);

		UE_LOG(LogTemp, Verbose, TEXT("ProceduralTreeComponent: Created leaf mesh section with %d verts, %d tris"),
		       LeafVertices.Num(), LeafTriangles.Num() / 3);
	}
}

void UProceduralTreeComponent::ApplyMaterials()
{
	// Apply bark material to section 0
	if (BarkMaterial)
	{
		SetMaterial(0, BarkMaterial);
	}

	// Apply leaf material to section 1
	if (LeafMaterial)
	{
		SetMaterial(1, LeafMaterial);
	}
}
