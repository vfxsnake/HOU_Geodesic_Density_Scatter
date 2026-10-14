# Density-weighted Lloyd: the math

Reference for Milestone 2. Each formula lists its symbols and the Houdini equivalent.
Read in VS Code preview (Ctrl+Shift+V) or on GitHub; both render the LaTeX.

---

## 1. M1 centroid: plain average

$$\mathbf{c}_k = \frac{1}{|S_k|}\sum_{i \in S_k} \mathbf{P}_i$$

- $\mathbf{c}_k$: new position of seed $k$, (seed position at the end of the iteration).
- $S_k$: samples whose nearest seed is $k$ (`gds_cell` attribute equals $k$),  a list of samples closest to k, the samples who are lined to a specific see, meaning forming a cell arround the seed.
- $|S_k|$: number of samples in that cell, number of points linekd to the seed. lenght of the sample list.
- $\mathbf{P}_i$: position of sample $i$, Position of the ith sample point of the sample list.

**Houdini:** Attribute Promote point → detail, mode Average, once per cell. This is what
`gds_cell_center_position` does now.

## 2. M2 centroid: density-weighted average

$$\mathbf{c}_k = \frac{\displaystyle\sum_{i \in S_k} \rho_i \, \mathbf{P}_i}{\displaystyle\sum_{i \in S_k} \rho_i}$$

- $\rho_i$: painted density at sample $i$ (rho)

**Worked example:** a cell with 3 samples $a, b, c$, all with $\rho = 0.1$.

example, $\sum \rho_i$:

$$\mathbf{c}_k = \frac{0.1\,\mathbf{P}_a + 0.1\,\mathbf{P}_b + 0.1\,\mathbf{P}_c}{0.1 + 0.1 + 0.1}$$

If every $\rho_i$ is equal, $\rho$ cancels and this becomes formula 1. High-density samples pull the
centroid toward themselves.

**Houdini:** the same average, but each sample counts $\rho_i$ times.

## 3. The proportional sign $\propto$

$$a \propto b \quad\Longleftrightarrow\quad a = C\,b \ \text{ for some constant } C$$

Only ratios matter. Comparing two places $A$ and $B$, the constant $C$ cancels:

$$h \propto \rho^{-1/4} \quad\Longrightarrow\quad \frac{h_A}{h_B} = \left(\frac{\rho_A}{\rho_B}\right)^{-1/4}$$

**Houdini:** a ramp whose curve shape you know but whose output range you don't. $C$ depends on
point count and surface area.

## 4. How spacing responds to density

Weighted Lloyd (formula 2) on a surface:

$$h(\mathbf{x}) \propto \rho(\mathbf{x})^{-1/4} \qquad \rho_A = 16\,\rho_B \;\Rightarrow\; \frac{h_A}{h_B} = 16^{-1/4} = \frac{1}{2}$$

Capacity-constrained: every cell holds the same mass $m_k$.

$$m_k = \sum_{i \in S_k} \rho_i = \frac{1}{n}\sum_{\text{all } i} \rho_i$$

$$h(\mathbf{x}) \propto \rho(\mathbf{x})^{-1/2} \qquad \rho_A = 16\,\rho_B \;\Rightarrow\; \frac{h_A}{h_B} = 16^{-1/2} = \frac{1}{4}$$

- $h(\mathbf{x})$: local spacing between seeds near point $\mathbf{x}$
- $m_k$: mass of cell $k$, the total density it covers
- $n$: number of seeds

**Why it matters:** with plain weighting the artist paints 16× and gets only 2× tighter spacing.
Equal mass per cell gives the 4× they expect.

## 5. Samples scattered by density

Draw the samples with probability proportional to density:

$$p(\mathbf{x}) \propto \rho(\mathbf{x})$$

Then the plain average (formula 1) over those samples estimates the weighted centroid (formula 2):

$$\frac{1}{|S_k|}\sum_{i \in S_k} \mathbf{P}_i \;\approx\; \frac{\displaystyle\int_{V_k} \rho(\mathbf{x})\,\mathbf{x}\,dA}{\displaystyle\int_{V_k} \rho(\mathbf{x})\,dA}$$

- $p(\mathbf{x})$: probability of a sample landing at $\mathbf{x}$
- $V_k$: the region of surface that belongs to seed $k$ (its Voronoi cell)
- $dA$: a small patch of surface area

Density goes into *where the samples land* instead of into a weight. Small cells in dense regions
also get more samples, which helps with the M1 sample-noise floor.

**Houdini:** Scatter SOP with Density Attribute set to the painted `density`, on the samples only.

Formulas 2 and 5 are **the same target computed two ways**: both are weighted Lloyd, both give
$h \propto \rho^{-1/4}$. They differ only in noise.

## 6. Fixing the weak response: remap the density

Feed the squared density in as the weight, in formula 2 or 5:

$$\rho' = \rho^2 \quad\Longrightarrow\quad h \propto \left(\rho^2\right)^{-1/4} = \rho^{-1/2}$$

- $\rho'$: the weight the algorithm sees; $\rho$ stays what the artist painted

Same spacing response as capacity-constrained (section 4), without a new algorithm.

## 7. Capacity-constrained swap (Balzer et al. 2009)

With density-scattered samples (section 5) every sample carries equal mass, so equal mass per cell
means equal sample count per cell:

$$|S_k| = \frac{N}{n} \quad \text{for every cell } k$$

Assignment is no longer "nearest seed". Start from any assignment with equal counts, then for two
neighboring seeds $\mathbf{s}_a$ and $\mathbf{s}_b$, swap sample $x$ (in cell $a$) with sample $y$
(in cell $b$) if it lowers the total squared distance:

$$\Big(\|\mathbf{P}_x - \mathbf{s}_a\|^2 - \|\mathbf{P}_x - \mathbf{s}_b\|^2\Big) + \Big(\|\mathbf{P}_y - \mathbf{s}_b\|^2 - \|\mathbf{P}_y - \mathbf{s}_a\|^2\Big) > 0$$

Counts stay equal because samples move in pairs. Repeat until no pair swaps, then move each seed to
the plain centroid of its samples (formula 1).

- $N$: total samples, $n$: total seeds
- $\mathbf{s}_a$: position of seed $a$
- $\|\mathbf{v}\|^2$: squared length of $\mathbf{v}$ (`length2` in VEX)
- first bracket: how much closer $x$ is to $b$ than to $a$; second: how much closer $y$ is to $a$

**Houdini:** no SOP equivalent. Sequential: runs in a detail wrangle.
