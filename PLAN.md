# Atom Probability Flow Simulator — Improvement Plan

## Current State Assessment

The simulation is a solid foundation: it samples quantum wavefunctions via CDF inversion, renders particles as 3D spheres with a fire-heatmap colormap, and animates probability current flow in real time. However there are several correctness bugs, performance bottlenecks, and missing features worth addressing.

---

## 🐛 Bug Fixes (Priority: Critical)

### 1. Broken `m < 0` Support
**Problem:** `sampleTheta` computes `P_l^m(cosθ)` using only `m ≥ 0`. Negative `m` values silently produce garbage distributions.  
**Fix:** Add `abs(m)` everywhere in the Legendre calculation. Real spherical harmonics for `m < 0` use a sin-based linear combination — implement `Y_lm_real` properly.

### 2. Stale CDF Caches
**Problem:** Both `sampleR` and `sampleTheta` use `static bool built` guards, so changing `n`, `l`, or `m` at runtime never rebuilds the CDFs. Every key-press update still draws from the *old* distribution.  
**Fix:** Hash `(n, l, m)` and rebuild when the hash changes. Or simply remove `static` and pass CDFs as parameters managed by `generateParticles`.

### 3. Incorrect Normalization in `sampleR`
**Problem:** `tgamma(n - l)` will call `tgamma(0)` when `n == l` (i.e., `k = 0`, the 1s-like case for each `n`). `tgamma(0)` is ±∞.  
**Fix:** Guard with `k = n - l - 1; if (k < 0) k = 0;` and use the correct normalization formula for hydrogen-like wavefunctions from NIST / Griffiths.

### 4. Probability Current Ignores `m = 0` Case
**Problem:** `calculateProbabilityFlow` divides by `m`, so `m = 0` gives a zero velocity vector — correct physically — but particles with `m = 0` still "rotate" due to floating-point noise. Also, `m < 0` gives backward rotation; this may or may not be intentional.  
**Fix:** Early-return `vec3(0)` for `m == 0`. Document that `|m|` drives the rotation speed and sign determines direction.

### 5. `inferno()` Heuristic Scaling Is Broken for Large `n`
**Problem:** `intensity * 1.5 * pow(5, n)` overflows to `inf` for `n ≥ 6` or so, washing everything white.  
**Fix:** Normalize intensity relative to the analytical peak of `|ψ|²` for the given `(n, l, m)`, or clamp after computing a per-frame max intensity.

---

## ⚡ Performance Improvements

### 6. Instanced Rendering
**Problem:** Each particle issues a separate `glDrawArrays` call — 250,000 draw calls per frame is the main GPU bottleneck.  
**Fix:** Use `glDrawArraysInstanced` with a per-instance SSBO or instanced VBO containing positions and colors. This typically gives a **10–50× speedup**, enabling 1M+ particles.

### 7. GPU-Side Probability Current (Compute Shader)
**Problem:** The velocity/position update loop runs on the CPU, forcing 250,000 trig calls per frame.  
**Fix:** Move the update to a GLSL compute shader. Each invocation handles one particle; the result feeds directly into the instanced VBO.

### 8. Parallel CDF Inversion on Startup
**Problem:** `generateParticles` is single-threaded. At N=250,000 it creates a noticeable stall.  
**Fix:** Use `std::async` / `std::execution::par_unseq` (C++17) to fill the particle array in parallel, then upload to GPU in one `glBufferData`.

### 9. Adaptive Level-of-Detail for Sphere Geometry
**Problem:** Every particle uses the same 10×10 sphere. Far particles waste triangles; close particles look faceted.  
**Fix:** Pre-generate 3 sphere LODs (4×4, 8×8, 16×16). Select based on distance to camera during instanced draw.

---

## 🎨 Visual & UX Improvements

### 10. Cross-Section / Slice Mode
Add a keyboard toggle (e.g. `C`) that culls particles outside a thin XZ or XY slab, revealing the internal nodal structure — the most physically informative view of higher orbitals.

### 11. Nodal Surface Overlay
Render semi-transparent isosurfaces (marching cubes on a 3D grid) at `|ψ|² = 0` to explicitly show radial and angular nodes. This makes the quantum structure immediately legible.

### 12. Axis & Scale Indicators
Render labelled axes (`x`, `y`, `z`) and a scale bar in Bohr radii (`a₀`). Currently there is no spatial reference — users cannot tell how large the orbital is.

### 13. HUD Display
Render the current `(n, l, m)` quantum numbers, orbital name (e.g. `3d₀`), number of particles, and FPS as on-screen text using a bitmap font or `stb_truetype`. Right now this info only appears in the terminal.

### 14. Smooth Quantum Number Transitions
When the user presses a key, instead of a hard particle reset, blend between old and new distributions over ~0.5 s by linearly interpolating particle positions toward newly sampled targets.

### 15. Better Colormap Options
Add 2–3 colormap presets (inferno, viridis, phase-angle-based RGB) switchable at runtime. Phase-angle coloring (hue = `arg(ψ)`) is especially informative for complex wavefunctions.

### 16. Window Resizing Support
Currently projection uses hardcoded `800/600`. Hook `glfwSetFramebufferSizeCallback` to update the projection matrix and `glViewport` on resize.

---

## 🧪 Physics Extensions

### 17. Full Complex Wavefunction (Phase Animation)
Currently only `|ψ|²` is visualized. Store the complex phase `e^{imφ}` per particle and animate the hue to show the rotating phase — physically accurate and visually striking.

### 18. Hydrogen-Like Ions (Variable Z)
Add a nuclear charge parameter `Z` (default 1). Scale all lengths by `1/Z`, letting users explore He⁺, Li²⁺, etc. with a single keypress.

### 19. Superposition States
Allow linear combinations `ψ = α|n₁l₁m₁⟩ + β|n₂l₂m₂⟩`. Sample from the mixed density `|ψ|²`. Visualize interference and beating between states.

### 20. Wavefunction Export
Add an `F5` shortcut that evaluates `|ψ(r,θ,φ)|²` on a 3D grid and writes a `.vti` (VTK image) or `.npy` file for external analysis in ParaView or Python.

---

## 🏗️ Code Architecture

### 21. Fix Global State / Statics
`n`, `l`, `m`, `N`, `gen`, `dis`, and the static CDF vectors are all unencapsulated globals. Wrap them in a `QuantumState` struct and pass by reference. This also fixes the stale-cache bug (#2).

### 22. Shader Error Reporting
The shaders are compiled with no error checking. Add a `checkShaderCompile(GLuint shader, const char* name)` helper that calls `glGetShaderiv(GL_COMPILE_STATUS)` and prints the info log. Same for link errors.

### 23. Split Into Files
The entire simulation is ~500 lines in one `.cpp`. Suggested split:
```
src/
  main.cpp          — engine loop only
  physics.cpp/.h    — sampling, probability current, wavefunction math
  renderer.cpp/.h   — OpenGL, instancing, shaders
  camera.cpp/.h     — camera struct
  grid.cpp/.h       — grid rendering
```

---

## Suggested Implementation Order

| Phase | Items | Goal |
|-------|-------|------|
| **1 — Correctness** | #1, #2, #3, #4, #5 | Physically accurate for all valid `(n,l,m)` |
| **2 — Performance** | #6, #7, #8 | 60 fps at 1M particles |
| **3 — Usability** | #10, #12, #13, #16 | Navigable, readable output |
| **4 — Visual** | #11, #14, #15 | Publication-quality visuals |
| **5 — Physics+** | #17, #18, #19 | Research/education tool |
| **6 — Architecture** | #21, #22, #23 | Maintainable codebase |
