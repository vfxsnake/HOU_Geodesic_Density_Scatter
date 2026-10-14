# Milestone 1 — Euclidean Lloyd relaxation (VEX prototype)

**Rung policy:** build for the end result. The loop is small; the rungs that
matter (density, geodesic) are their own milestones.

## Where we are
Nothing built yet. This milestone builds the core loop every later milestone modifies:
`mesh → seeds + samples → [assign samples to nearest seed → move seed to centroid → project] × N`.
M2 changes the centroid weights, M3 changes the distance used in "nearest", M4/M5 port the loop to
OpenCL. Scene: `hip/m01_lloyd_euclidean.hipnc`.

## Goal
Turn a clumpy random scatter on a surface into evenly spaced
points with a discrete Lloyd loop in VEX.

## Exit condition
- **Seen:** after N iterations the seeds look evenly spaced: no clumps, no holes. Comparable to (not
  identical to) Scatter with Relax Iterations on.
- **Measured:** stddev of nearest-neighbor distance drops versus the raw scatter and stops changing
  as iterations increase (convergence). Mean seed displacement per iteration trends to ~0.

## Concepts

### Voronoi cell
The part of the surface closer to a given seed than to any other seed. Houdini anchor: Voronoi
Fracture cells, restricted to the surface.

### Lloyd iteration
Move each seed to the centroid of its cell, repeat. Fixed point = centroidal Voronoi tessellation
(CVT): every seed sits at the center of mass of its own cell → even spacing.

### Discrete Lloyd (what we actually compute)
Cells are not computed explicitly. A dense, fixed **sample cloud** stands in for the surface area:
- Each sample is a small piece of area.
- **Assignment:** each sample stores the index of its nearest seed → `i@cell`. That labelling *is* the
  Voronoi diagram, sampled.
- **Update:** seed position = average `P` of the samples labelled with its index.
  `centroid = (1/n) · Σ P_sample`. M2 turns this into `Σ w·P / Σ w`.
- **Project:** the average of surface points lies *inside* a curved surface → snap back with
  `xyzdist` + `primuv`.

## Design decisions
| Decision | Choice | Status | Reason |
|----------|--------|--------|--------|
| Relaxation method | Discrete Lloyd (sample assignment) | Agreed | M2 = weights, M3 = metric — one-line changes |
| Initial scatter | Scatter SOP, Relax off | Agreed | The clumpy input is the problem we solve |
| Sample cloud | Scatter SOP, Relax off, fixed | Agreed | Using Scatter's relax would be circular. Random samples are unbiased but noisy (~25 per cell at 50k:2k) — sample count is a measurable knob. Rejected: Relax on (circular), prim centers × `@area` (mesh-tied resolution). M2: density goes in weights, never in sample placement |
| Generate-while-relaxing (Poisson / dart throwing) | Rejected | Agreed | Sequential (bad for GPU), only a min-distance, not a CVT |
| Loop | For-Loop with Feedback (Block Begin / Block End) | Agreed | Same shape as the compiled block in M5 |
| Seed→samples lookup | `findattribval` on `i@cell` | Agreed | Simplest in VEX; O(samples) total per iteration |
| Empty cell | Seed keeps its position | Agreed | No samples → no centroid |
| Better init (seed scatter relaxed) | Optional experiment | Agreed | Fewer iterations to converge — measure it |

## Implementation steps
Each step: the action, then the check that proves it worked. Predict before every cook.

- [x] 0. **Setup:** create `hip/m01_lloyd_euclidean.hipnc`. Verify a wrangle can `#include` a file
      from `vex/include/` → wrangle compiles and calls it. *Include path solved:* package JSON
      (`hpath` → project root) puts the repo on `HOUDINI_PATH`; vcc searches `vex/include` under
      every `HOUDINI_PATH` entry. Test header works (0, 10, 20…).
- [x] 1. **Spike:** test mesh (sphere or torus, ~2–5k polys) → Scatter A (seeds, ~2k, Force Total
      Count, Relax off) and Scatter B (samples, ~50k, different Global Seed, Relax **off** — no dependence on Scatter's relax). Sample wrangle:
      `i@cell = nearpoint(<seeds input>, @P)`, color samples by a random color from `cell`
      → the viewport shows the sampled Voronoi diagram as color patches.
      **Break it:** feed the *samples* themselves as the seed input → predict what the coloring
      shows, then check.
- [x] 2. **One Lloyd step (no loop):** seed wrangle reads samples with `cell == @ptnum`, averages
      their `P` → seeds move. Before projecting, check: on a sphere, are moved seeds slightly inside
      the surface? (`xyzdist` value > 0.)
      *Done: big cells (≥60 samples) move ~1.6× more than small (≤5). Off-surface sign check skipped by
      choice — projection is a separate wrangle, bypass it to see the drift.*
- [x] 3. **Project:** `xyzdist` to the mesh, `primuv(..., "P", ...)` → seeds back on the surface
      (distance ≈ 0).
      *Done: distance to the mesh before projecting: max 0.0167, median 0.0009. After: 0 for all 2000 seeds.*
- [x] 4. **Loop:** wrap steps 1–3 (assign + update + project) in Block Begin (Fetch Feedback) /
      Block End (Feedback Each Iteration). Iterations as a `ch()` slider. Samples and mesh enter
      from outside the loop → drag the iteration count: spacing evens out, change slows down.
      *Done: most movement happens in iterations 1–13, only a few visible moves after that.*
- [x] 5. **Move code into `vex/include/`:** wrangles become include + call.
      *Done: `vex/include/gds_lloyd.h` — `gds_assign_euclidean`, `gds_cell_center_position`,
      `gds_project_to_surface`; cell attribute name is `GDS_CELL_ATTRIB` ("gds_cell"), written with
      `setpointattrib`. Converted in place: no inline A/B kept, equality checked visually only.*
- [x] 6. **Measure:** a stats wrangle: per-seed nearest-neighbor distance (`nearpoints(..., 2)` and take
      the second hit, the first is itself) → detail mean / stddev. Record raw scatter vs. N = 5, 10,
      20, 50. Also mean displacement per iteration (store `P` before the step, compare after).
      Results (rubber toy, 5k seeds, NN distance):

      | N  | mean    | stddev   | stddev / mean |
      |----|---------|----------|---------------|
      | 0  | 0.01955 | 0.01005  | 0.514         |
      | 5  | 0.03233 | 0.00605  | 0.187         |
      | 10 | 0.03278 | —        | —             |
      | 20 | 0.03291 | 0.00559  | 0.170         |
      | 30 | 0.03293 | 0.00556  | 0.169         |

      Most of the gain comes by N = 5; plateau ≈ 0.17 from N ≈ 20 (topological defects + sample noise, not 0).
      Displacement per iteration: skipped by choice — exponential decay observed visually, not measured.
- [x] 7. **Reference compare:** Scatter with Relax Iterations on, same seed count → stats side by side.

      | N  | Lloyd stddev/mean | Scatter relax stddev/mean |
      |----|-------------------|---------------------------|
      | 0  | 0.514             | 0.514                     |
      | 5  | 0.187             | 0.149                     |
      | 20 | 0.170             | 0.113                     |
      | 30 | 0.169             | 0.108                     |

      Scatter relax wins and is still improving at N = 30. Raising samples 100k → 400k lowers the Lloyd
      plateau: centroid noise from samples per cell (~20) sets the floor. Not dug further. Matters again
      in M2, where dense regions get fewer samples per cell.
      400k samples **with Relax Points on** → Lloyd 0.105 at N = 30, slightly below Scatter's 0.108.
      Evenly spread samples beat random ones for centroid estimates (count and relax changed together,
      not separated).
- [~] 8. *(Optional, skipped)* **Better init:** relaxed seed scatter as the loop input → iterations to reach the
      same stddev.

## Reference

### Functions (checked against the installed Houdini 22 docs)
- `int nearpoint(<geometry>, vector pt)`: index of the closest point. Assignment step.
- `int[] nearpoints(<geometry>, vector pt, float maxdist, int maxpts)`: k nearest. Spacing stats.
- `int findattribvalcount(<geometry>, "point", "cell", value)` +
  `int findattribval(<geometry>, "point", "cell", value, index)`: iterate the samples of one cell.
  The docs show this loop pattern exactly.
- `float xyzdist(<geometry>, vector pt, int &prim, vector &uv)`: closest surface location.
- `primuv(<geometry>, "P", prim, uv)`: position at that location.
- Block End **Gather Method → Feedback Each Iteration**. Block Begin **Method → Fetch Feedback**.

### Inputs per wrangle
| Wrangle | Runs over | Input 0 | Input 1 | Input 2 |
|---------|-----------|---------|---------|---------|
| assign  | samples | samples | seeds (loop feedback) | — |
| update  | seeds | seeds | samples with `cell` | mesh |
| stats   | seeds | seeds | — | — |

### VEX headers
- Lookup: `#include "x.h"` → `vex/include/` under each `HOUDINI_PATH` entry (quotes also search the including file's dir first).
- Guard with `#ifndef` / `#define` / `#endif` — `#pragma once` is not in the vcc docs.
- Parameters: `,` shares the previous type (`int a, b`), `;` starts a new typed one (`int a; float b`).
- No namespaces — prefix every function `gds_`.
- Errors: "Invalid source" is the wrapper; the detail line says *unable to include* (not found) vs. *syntax error (line,col)* (found, broken inside the header).

### Pitfalls
- **Sample noise:** too few samples per cell → converged seeds jitter. Experiment: double the sample count, compare NN-distance stddev.
- **Point numbers as cell IDs:** `cell` stores the seed's `@ptnum`. Valid only while the seed count and
  order don't change inside the loop. Don't delete or sort seeds in the loop.
- **Stale input:** the assign wrangle must read seeds from *inside* the loop (feedback), not from the
  original scatter. If it reads the original, the seeds move once and then stop.
- **Sample density = quality ceiling:** too few samples per seed (<~10) → noisy centroids, seeds jitter
  and never settle. That shows up as displacement that plateaus above 0.
- **Euclidean nearest across gaps:** on a thin-gap mesh, samples get assigned to a seed on the other
  side. That's expected; it's the M3 failure case. Use a sphere or torus here.
- **Boundaries:** on open meshes, edge cells are cut off, so seeds pull inward slightly. That's expected for a CVT.

## Network / files

### `vex/include/gds_lloyd.h`
**Responsibility:** assign-to-nearest, centroid-of-cell, project-to-surface functions.

### Nodes: `/obj/m01/…`
**Inputs / outputs:**
- samples: `i@gds_cell` (point, name from `GDS_CELL_ATTRIB`), written each iteration.
- seeds: `P` (point), updated. `v@P_prev` (point), for displacement.
- stats: `f@nn_dist` (point). `f@nn_mean` and `f@nn_std` (detail).
