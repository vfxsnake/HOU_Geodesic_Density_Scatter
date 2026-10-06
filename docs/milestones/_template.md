# Milestone N — <Title>

**Rung policy:** <walk the rungs / build for the end result>

## Where we are
3–6 lines: the network as a sketch, which box this milestone fills, which earlier pieces it reuses.
Read this at the start of every session in the milestone.

## Goal
One sentence.

## Exit condition
What can be seen in the viewport or measured to confirm the milestone is complete.

## Concepts
### <Concept name>
What the quantity *means*, then the formula. Houdini anchor (the SOP / VEX equivalent).

## Design decisions
| Decision | Choice | Status | Reason |
|----------|--------|--------|--------|
|          |        |        |        |

## Implementation steps
Ordered. Each step: the action, then the check that proves it worked.
- [ ] 1. **Spike:** <smallest network that shows the effect> → <expected viewport result>.
      Break it: <deliberate break> → <predicted failure>.
- [ ] 2. <action> → <expected result in viewport / measurement>

## Reference
Keep at hand while implementing.
- **Concepts / formulas:** meaning first, then the expression.
- **Functions & parameters:** VEX / OpenCL functions and SOP parameters used, verified against the
  installed Houdini docs.
- **Pitfalls:** what is likely to go wrong and how it shows up.

## Network / files
### `<kernels/name.cl | vex/name.vfl | node path>`
**Responsibility:** ...
**Inputs / outputs:** attributes read and written, and their class (point / prim / vertex / detail).

## Benchmark (milestones 4–5)
| Run | Points | Iterations | Viewport state | Time | Notes |
|-----|--------|------------|----------------|------|-------|
