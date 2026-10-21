# Geodesic Density Scatter

OpenCL point relaxation for surface scatter in Houdini: density-weighted Lloyd relaxation on a
geodesic metric, running GPU-resident inside a compiled block. Built with the OpenCL SOP and compiled
blocks only — no HDK.

This is a learning project with two goals:

1. **Scatter quality and artist control** — geodesic (not euclidean) relaxation so points don't repel
   across thin gaps, plus a painted density map that tightens local spacing (weighted Lloyd /
   capacity-constrained CVT).
2. **GPU residency** — demonstrate how a naive OpenCL port breaks residency every iteration, and how
   restructuring inside a compiled block fixes it. The naive version is kept as the benchmark baseline.

---

## Current status

**As of 2026-10-20 — Milestone 1 complete; Milestone 2 in progress (steps 1–4 done; next: revisit the γ remap step).**

- **Milestone 1 (VEX prototype, euclidean): done.** Lloyd loop in `vex/include/gds_lloyd.h`. Nearest-neighbor stddev / mean 0.51 raw → plateau ≈ 0.17. Scatter's built-in relax reaches 0.11 at 30 iterations; the Lloyd floor comes from centroid sample noise — 400k relaxed samples bring it to 0.105, level with Scatter.
- **Milestone 2 (density weighting): in progress** (`docs/milestones/m02_density_weighting.md`). Test density in `vex/include/gds_density.h`: point falloff and axis gradient, each with a `(1 − t)^k` decay that keeps the min/max values exact; a Switch picks one. Samples carry `density` → `gds_weight`; `gds_cell_center_weighted` (Σ w·P / Σ w, Σ w = 0 keeps the seed) wired into the loop beside the plain centroid via a Switch — seeds pack toward high density; scaling all weights changes nothing (only ratios matter). Density 0 means no points: seeds and samples with density 0 are deleted before the loop; a `density_min` floor covers near-zero weights. Seeds re-read `density` from the mesh after the loop (`gds_density_at_surface`); a `chramp` band gates the spacing stats. Measured (16:1 density): from uniform seeds the blob/background spacing ratio freezes at 0.965 by 128 iterations — Lloyd stops at the nearest resting state, since constant weight inside a cell gives the plain centroid. From density-scattered seeds it holds ≈ 0.23 (raw 0.25) while stddev/mean drops 0.50 → 0.12: the seed start sets the density response, Lloyd evens the spacing. The predicted ρ^(-1/4) (≈ 0.5) is not reproduced. Rungs: weighted centroid → ρ² remap → density-scattered samples (A/B) → capacity-constrained. Math in `docs/notes/density_weighted_lloyd.md`.

Known gaps: none yet.

---

## Architecture

```
HOU_Geodesic_Density_Scatter/
├── kernels/     ← OpenCL kernels (*.cl), loaded by the OpenCL SOP from file
├── vex/include/ ← VEX headers, #included by wrangles
├── hip/         ← demo.hipnc (final scene; milestone scenes stay local)
├── otls/        ← HDA
├── geo/         ← test geometry
├── bench/       ← benchmark results and profiler summaries
└── docs/
```

### The two residency breakers

1. **Geodesic solve** — heat method via Linear Solver SOP forces a CPU readback. Precompute once vs.
   k-ring approximation in-kernel.
2. **Neighbor rebuild** — per-iteration `pcopen` forces a readback. GPU grid in-kernel vs. fixed
   neighborhood rebuilt every K iterations.

---

## Environment

| Item | Value |
|------|-------|
| Houdini | 22 Apprentice |
| OS | Windows |
| GPU | NVIDIA RTX 2070 8GB |
| Demo target | 2M points |

---

## Milestones

| # | Milestone | Status |
|---|-----------|--------|
| 1 | VEX prototype — euclidean Lloyd, no density | Done |
| 2 | Density weighting | In progress |
| 3 | Geodesic metric — thin-gap demo | Not started |
| 4 | OpenCL port, naive — benchmark + profiler readbacks | Not started |
| 5 | Residency restructure in compiled block — benchmark | Not started |
| 6 | HDA with artist-facing parameters | Not started |
