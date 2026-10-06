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

**As of 2026-10-06 — Milestone 1 in progress (step 1 of the plan).**

- **Milestone 1 (VEX prototype, euclidean): plan agreed; VEX include path working (package → `HOUDINI_PATH`, headers in `vex/include/`); sampled Voronoi assignment builds (2k seeds / 50k samples, both unrelaxed).**

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
| 1 | VEX prototype — euclidean Lloyd, no density | In progress |
| 2 | Density weighting | Not started |
| 3 | Geodesic metric — thin-gap demo | Not started |
| 4 | OpenCL port, naive — benchmark + profiler readbacks | Not started |
| 5 | Residency restructure in compiled block — benchmark | Not started |
| 6 | HDA with artist-facing parameters | Not started |
