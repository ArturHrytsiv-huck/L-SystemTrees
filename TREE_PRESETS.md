# L-System Tree Presets

## Fruit Tree Formulas

### 1. Apple Tree (Recommended)
Rounded canopy with spreading horizontal branches.

```
Axiom: FA
Rules:
  A -> [&FL]////[&FL]////[&FL]////[&FL]////[&FL]
  F -> S//[--L][++L]F
  S -> S[//&&L][//^^L]

Iterations: 4
DefaultAngle: 30
PitchAngle: 25
RollAngle: 72 (360/5 for 5-way symmetry)
StepLength: 15
WidthFalloff: 0.65
```

**Simplified Apple Tree (single rule):**
```
Axiom: F
Rule: F -> FF[&&-FL][&&+FL][&---L][&+++L]
Iterations: 4
DefaultAngle: 25
PitchAngle: 35
```

### 2. Cherry/Peach Tree
More upright with graceful drooping branch tips.

```
Axiom: F
Rule: F -> FF&[+F-FL]&[-F+FL]
Iterations: 5
DefaultAngle: 28
PitchAngle: 15
StepLength: 12
WidthFalloff: 0.7
```

### 3. Orange/Citrus Tree
Dense, bushy, rounded shape with many small branches.

```
Axiom: FA
Rules:
  A -> [+FA][-FA][^FA][&FA]
  F -> FF

Iterations: 4
DefaultAngle: 35
PitchAngle: 35
StepLength: 8
WidthFalloff: 0.75
BranchProbability: 0.85
```

### 4. Weeping Tree (Willow-like)
Drooping branches.

```
Axiom: F
Rule: F -> FF[&&+F&&+F&&L][&&-F&&-F&&L]
Iterations: 5
DefaultAngle: 20
PitchAngle: 25
StepLength: 10
TropismStrength: 0.3
```

### 5. Realistic Fruit Tree (Complex)
Most natural-looking with multiple rules.

```
Axiom: FFFA
Rules:
  A -> [&B]////[&B]////[&B]
  B -> F[--L][++L]FA
  F -> S
  S -> F

Iterations: 5
DefaultAngle: 25
PitchAngle: 30
RollAngle: 120 (360/3 for 3-way branching)
StepLength: 12
InitialWidth: 8
WidthFalloff: 0.6
AngleVariationMin: -10
AngleVariationMax: 10
StepLengthVariation: 0.15
BranchProbability: 0.9
```

---

## Symbol Reference

| Symbol | Action |
|--------|--------|
| F | Move forward, draw branch |
| f | Move forward, no branch |
| + | Turn right (yaw) |
| - | Turn left (yaw) |
| ^ | Pitch up |
| & | Pitch down (toward ground) |
| \ | Roll clockwise |
| / | Roll counter-clockwise |
| \| | Turn around 180 |
| [ | Save state (start branch) |
| ] | Restore state (end branch) |
| L | Place leaf |
| A,B,S,X,Y,Z | Variables (no action, used in rules) |

---

## Tips for Natural Fruit Trees

1. **Use `&` (pitch down)** to make branches spread outward, not just sideways
2. **Multiple roll symbols** (`////`) create radial branch distribution around trunk
3. **Lower angles** (20-30) look more natural than 45
4. **Add randomness**:
   - `AngleVariationMin: -8`
   - `AngleVariationMax: 8`
   - `BranchProbability: 0.85`
5. **Tropism** (0.1-0.2) makes branches droop realistically
6. **Use variables** (A, B) for multi-stage growth patterns

---

## Quick Copy-Paste for Editor

### Best Fruit Tree Setup:

**Axiom:** `FFFA`

**Rule 1:**
- Predecessor: `A`
- Successor: `[&B]////[&B]////[&B]`

**Rule 2:**
- Predecessor: `B`
- Successor: `F[--L][++L]FA`

**Rule 3:**
- Predecessor: `F`
- Successor: `S`

**Rule 4:**
- Predecessor: `S`
- Successor: `F`

**Turtle Config:**
- Default Angle: 22
- Pitch Angle: 35
- Roll Angle: 120
- Step Length: 12
- Initial Width: 6
- Width Falloff: 0.62
- Angle Variation Min: -8
- Angle Variation Max: 8
- Branch Probability: 0.88
- Tropism Strength: 0.05
