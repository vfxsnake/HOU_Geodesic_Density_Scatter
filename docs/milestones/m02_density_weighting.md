# Milestone 2 — Density weighting

**Rung policy:** walk the rungs. Weighted centroid → ρ² remap → density-scattered samples (A/B) →
capacity-constrained. Each rung is compared against the one before it.

## Where we are
M1 built the loop: `mesh → seeds + samples → [assign → centroid → project] × N`, euclidean, uniform.
M2 changes only the **centroid** box: each sample counts by its density. Assign and project stay.
The last rung (capacity-constrained) also changes **assign**. Scene: `hip/m02_density_weighting.hipnc`
(copy of m01). Math lives in `docs/notes/density_weighted_lloyd.md`; this plan references it by section.

## Goal
A painted density attribute controls local spacing: relaxed points pack tighter where density is high,
with a response close to `h ∝ ρ^(-1/2)`, and the point count stays fixed.

## Exit condition
- **Seen:** on the rubber toy with a test density (gradient + blob), points pack visibly tighter where
  density is high. No holes, no clumps, no seeds drifting out of low-density areas. Each rung is
  compared visually with the one before it.
- **Measured:** the existing measure chain (mean and stddev of NN distance), restricted to a density
  band by a ramp. Inside one band spacing should be even, so stddev/mean stays near M1's ≈0.17.
- **ρ = 0:** a zero-density region gives no division by zero and no NaN positions.

## Concepts
All formulas: `docs/notes/density_weighted_lloyd.md`.

### Weighted centroid (notes §2)
Each sample counts `ρ_i` times. The denominator `Σ ρ_i` rescales, so only *ratios* of ρ matter:
multiplying all ρ by a constant moves nothing. Houdini anchor: an Attribute Promote Average where
each point is weighted.

### Spacing response (notes §3, §4)
Plain weighting gives `h ∝ ρ^(-1/4)`: 16× density → 2× tighter. Artists expect `ρ^(-1/2)`: 16× → 4×.

### Remap (notes §6)
Weight by `ρ^γ` instead of ρ. `γ = 2` gives `h ∝ ρ^(-1/2)`. `γ` is a slider, so `γ = 1` is plain
weighting and the A/B is one drag.

### Density-scattered samples (notes §5)
Put density into *where the samples land* (Scatter's Density Attribute) and keep the plain average.
Same target as §2, less noise in dense cells (more samples per small cell).

### Capacity-constrained (notes §4, §7)
Every cell holds the same mass. With density-scattered samples every sample carries equal mass, so
"equal mass" = "equal sample count per cell". Assignment is no longer "nearest seed": samples are
swapped between neighboring cells while a swap lowers the total squared distance (notes §7).

## Design decisions
| Decision | Choice | Status | Reason |
|----------|--------|--------|--------|
| Test density | Procedural wrangle on the mesh: gradient along one axis + a blob, `f@density` (point) | Agreed | Reproducible for A/B. Attribute Paint for the artist demo later |
| Density onto samples | Scatter's **Point Attributes** (`pointattribs`, default `*`) interpolates `density` from the mesh | Agreed (verified) | Samples are fixed, so the lookup happens once, outside the loop |
| Weight | `f@gds_weight = pow(density, γ)` on the samples, `γ` as a `ch()` slider | Agreed | Precompute outside the loop; one slider covers rungs 1 and 3 |
| Weighted centroid | New function `gds_cell_center_weighted` in `gds_lloyd.h`; `gds_cell_center_position` stays | Agreed | Superseded code stays as A/B |
| `Σ w = 0` in a cell | Seed keeps its position (same rule as an empty cell) | Agreed | No mass → no centroid |
| Zero-density regions | `density_min` floor slider added before the remap | Agreed | Without it, seeds inside a zero region never get pulled out |
| Density in sample placement | A/B only (rung 4); revisits M1's "density goes in weights, never in sample placement" | Agreed | Same target, tests the noise claim |
| Capacity-constrained | Balzer-style swap, detail wrangle (sequential), on density-scattered samples; new header `gds_ccvt.h` | Agreed | Last rung; compared against γ = 2 |
| Band measure | Ramp on the seed's density masks which seeds accumulate; NN search still runs on all seeds | Agreed | Blasting first would inflate NN distance at the band edge |

## Implementation steps

- [ ] 0. **Setup:** save m01 as `hip/m02_density_weighting.hipnc`. Density wrangle on the mesh
      (gradient + blob), visualize as `Cd` → the gradient and blob show on the toy. Spreadsheet on the
      sample scatter → samples carry `density`.
- [ ] 1. **Spike:** samples get `f@gds_weight = density` (γ fixed at 1 for now).
      `gds_cell_center_weighted` → `Σ w·P / Σ w` (notes §2). Switch SOP between plain and weighted
      centroid → seeds pack toward the blob and the dense end of the gradient.
      **Break it:** invert the density (`1 - density`) → check.
      **Break it:** multiply density by 100 → check (notes §2: only ratios matter).
- [ ] 2. **ρ = 0:** set the blob to 0 instead of high → look at the seeds inside it.
      `Σ w = 0` fallback; `density_min` slider. Check: count of seeds with `isnan(@P.x)` = 0.
- [ ] 3. **Band measure:** seeds read their density from the mesh (`xyzdist` + `primuv`, as in
      projection). Measure wrangle: `chramp("band", density) > 0.5` gates the two `setdetailattrib`
      calls, and a third accumulates the in-band count; the stats wrangle divides by that count, not
      `@numpt`. Check: band = blob only → stddev/mean near ≈0.17.
- [ ] 4. **Weak response:** density 1/16 in the background, 1 in the blob. Band mean in the blob ÷ band
      mean in the background (notes §4 says ≈ 0.5). Record.
- [ ] 5. **Remap:** `γ` slider. γ = 2 → the same ratio (≈ 0.25 expected). Side by side with γ = 1.
      **Reference:** Scatter with Use Density Attribute + Relax on the same density. Its relax radius
      is ∝ `1/sqrt(density)` (Scatter docs), so its ratio should also be ≈ 0.25.
- [ ] 6. **Density-scattered samples (A/B):** sample Scatter with Use Density Attribute =
      `gds_weight`, same total count; centroid back to plain. Compare band stddev/mean in the blob
      with step 5 at the same γ → same ratio, lower stddev/mean expected.
- [ ] 7. **Capacity-constrained:** `gds_ccvt.h`, swap pass in a detail wrangle (notes §7). Start small
      (≈1k seeds / 20k samples) — it's sequential. Compare the ratio and band stats with step 5.

## Reference
Keep at hand while implementing.

### Functions & parameters (checked against the installed Houdini 22 docs)
- `float chramp(string channel, float ramppos)`: evaluates a ramp parameter; `ramppos` clamped to
  [0, 1]. Create the spare ramp parameter with the wrangle's "Create spare parameters" button.
- `int isnan(float)`: NaN check for step 2.
- `pow(x, y)`: the remap.
- `findattribval`, `pointattrib`, `xyzdist`, `primuv`, `nearpoints`, `setdetailattrib(..., "add")`:
  as in M1.
- **Scatter SOP:** `usedensityattrib` / `densityattrib` (Use Density Attribute / Density Attribute).
  With Force Total Count on, the attribute sets the *relative* distribution, not the absolute density.
  `pointattribs` (Point Attributes, default `*`) interpolates mesh point attributes onto the points.
- **Scatter relax with density:** relax radius ≈ `0.537 / sqrt(density)` on surfaces, so spacing
  `∝ ρ^(-1/2)`. Near-zero density → huge radius; Max Relax Radius limits it.

### Pitfalls
- **Global stddev rises with density working:** spacing is meant to vary, so measure per band.
- **Band edges:** never Blast before the NN search; gate only the accumulation.
- **Seed density vs. sample density:** the weight lives on the samples. Seeds read density only for
  the band measure.
- **Zero density:** `Σ w = 0` → division by zero → NaN position. NaN spreads: the next `nearpoint`
  assignment breaks for its neighbors.
- **Remap contrast:** γ = 2 squares the range (0.1–1 → 0.01–1). Low-density cells carry almost no
  weight and their seeds get pulled toward the dense side.
- **Samples per cell in dense regions:** with uniform samples, small cells get few samples → the M1
  noise floor, worse where it matters. Rung 6 is the fix.
- **Capacity-constrained cost:** the swap pass is sequential; it does not fit a point wrangle.
  Flag for M4/M5.

## Network / files

### `vex/include/gds_lloyd.h`
**Adds:** `gds_cell_center_weighted(input, seed_ptnum, seed_P)`, reads `f@gds_weight` from samples.

### `vex/include/gds_ccvt.h` (rung 7)
**Responsibility:** capacity-constrained swap pass.

### Nodes: `/obj/geo1/…`
**Inputs / outputs:**
- mesh: `f@density` (point), from the density wrangle.
- samples: `f@density` (point, interpolated by Scatter), `f@gds_weight` (point), `i@gds_cell` (point).
- seeds: `P` (point), `f@density` (point, for the band measure).
- stats: `f@near_neighbour_distance` (point); `accumulation`, `square_accumulation`, in-band count,
  `mean`, `standard_deviation` (detail).
