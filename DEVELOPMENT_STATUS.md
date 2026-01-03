# LSystemTrees Plugin - Development Status

**Last Updated:** January 2026
**Status:** Phase 3 Complete

## Overview

This plugin generates procedural 3D trees using L-System (Lindenmayer System) string rewriting and turtle graphics interpretation in Unreal Engine.

## Architecture

```
LSystemGenerator → TurtleInterpreter → TreeGeometry → ProceduralTreeComponent
                         ↓                  ↓
                     TreeMath           DebugDraw
```

## Implemented Components

### 1. Core Types (`LSystemTypes.h`)
- `FLSystemRule` - Production rules with stochastic probability
- `FLSystemGenerationResult` - Generation output with success/error info
- `FTurtleState` - Turtle position, orientation (Forward/Left/Up basis), width, depth
- `FTurtleConfig` - Interpretation parameters including randomness
- `FBranchSegment` - Branch segment with parent tracking for smooth connections
- `FLeafData` - Leaf placement data
- `FTreeMeshData` - Final mesh vertices/triangles/UVs
- `FTreeLODLevel` - LOD configuration
- `FTreeGeometryConfig` - Geometry generation settings

### 2. LSystemGenerator (`Core/LSystem/`)
- String rewriting with stochastic rules
- Configurable iterations and random seed
- Tested and working

### 3. TurtleInterpreter (`Core/TreeGeometry/`)
- Interprets L-System strings as 3D turtle graphics
- **Supported Symbols:**
  - `F` - Move forward, draw segment
  - `f` - Move forward, no drawing
  - `+/-` - Yaw rotation (around Up)
  - `^/&` - Pitch rotation (around Left)
  - `\/\/` - Roll rotation (around Forward)
  - `|` - Turn around 180°
  - `[/]` - Push/pop state (branching)
  - `L` - Place leaf
  - `X,Y,Z,A,B,G` - Ignored placeholders

- **Randomness Features (newly added):**
  - `BranchProbability` (0-1): Chance of branch creation
  - `AngleVariationMin/Max`: Random angle offset for rotations
  - `StepLengthVariation`: Random length variation for forward movement
  - Tropism (gravity influence on growth direction)

### 4. TreeGeometry (`Core/TreeGeometry/`)
- Generates tapered cylinders for branches
- Generates quads for leaves
- **Smooth branch connections**: Child segments reuse parent's end ring vertices
- Multiple LOD level support
- UV mapping for bark textures

### 5. TreeMath (`Core/Utilities/`)
- Vector rotation (Rodrigues' formula)
- Basis orthogonalization
- Perpendicular vector calculation

### 6. DebugDraw (`Core/Utilities/`)
- Branch segment visualization
- Leaf position/orientation display
- L-System string printing

### 7. ProceduralTreeComponent (`Components/`)
- Blueprint-spawnable component
- Combines all systems into one-click tree generation
- Separate mesh sections for bark (0) and leaves (1)
- Material slots for bark and leaf materials
- LOD switching support
- Editor property change auto-regeneration

## Key Files

```
Plugins/LSystemTrees/Source/LSystemTrees/
├── Public/
│   ├── Core/
│   │   ├── LSystem/
│   │   │   ├── LSystemGenerator.h
│   │   │   └── LSystemTypes.h          # All core data structures
│   │   ├── TreeGeometry/
│   │   │   ├── TurtleInterpreter.h
│   │   │   └── TreeGeometry.h
│   │   └── Utilities/
│   │       ├── TreeMath.h
│   │       └── DebugDraw.h
│   └── Components/
│       └── ProceduralTreeComponent.h
├── Private/
│   ├── Core/
│   │   ├── LSystem/
│   │   │   └── LSystemGenerator.cpp
│   │   ├── TreeGeometry/
│   │   │   ├── TurtleInterpreter.cpp
│   │   │   └── TreeGeometry.cpp
│   │   └── Utilities/
│   │       ├── TreeMath.cpp
│   │       └── DebugDraw.cpp
│   └── Components/
│       └── ProceduralTreeComponent.cpp
└── LSystemTrees.Build.cs
```

## Default Tree Configuration

**Default Rule:** `F → FF&[-/F+F+FL]^[+\F-F-FL]`

This creates a 3D tree with:
- Double trunk growth per iteration
- Pitch down (`&`) before left branch, pitch up (`^`) before right branch
- Roll (`/` and `\`) for branch rotation variety
- Leaves (`L`) at branch tips

## Randomness Parameters (Editor-Editable)

Under `Tree|Turtle` category:
- **Branch Probability** (0.8 default): 80% chance each branch is created
- **Angle Variation Min** (0.0 default): Minimum random angle offset
- **Angle Variation Max** (0.0 default): Maximum random angle offset
- **Step Length Variation** (0.0 default): Random length variation (0-1 range)

To get varied trees: Change `RandomSeed` or set to 0 for random seed each generation.

## Known Issues / Future Work

1. **LOD auto-switching**: Currently manual via `SetLODLevel()`. Could add distance-based automatic switching.
2. **Wind animation**: Could add vertex animation for wind effects.
3. **Collision**: No collision generated currently.
4. **Async generation**: Large trees could benefit from async mesh generation.

## Testing

1. Add `ProceduralTreeComponent` to any Actor
2. Configure Axiom, Rules, Iterations in Details panel
3. Set materials for Bark and Leaves
4. Call `GenerateTree()` or enable `Generate On Start`
5. Use `DrawDebug()` for visualization

## Recent Changes (This Session)

1. Fixed flat tree issue - Added pitch (^/&) and roll (\//) to default rule
2. Fixed cylinder gaps - Implemented parent segment tracking and vertex reuse
3. Added randomness parameters:
   - BranchProbability for random branch skipping
   - AngleVariationMin/Max for rotation randomness
   - StepLengthVariation for length randomness
   - All editable in editor under Turtle Config
