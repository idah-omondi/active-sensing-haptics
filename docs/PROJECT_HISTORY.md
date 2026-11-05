# Project history

How the experiment and the analysis reached their final form.

## Stage 0, device and scene

Set up OpenHaptics 3.5.0 with Visual Studio and a Touch X. Rebuilt the lab's visual tracking demo, where a subject follows a moving box with a second box, and added logging. Several small sandbox programs explored haptic fading (`haptics/sandbox_ghost_object`).

## Stage 1, first pilot (`archive/pilot_velocity_divisor`)

Visibility was |v_q| divided by a constant, clamped to [0, 1], with a 5 mm/s deadband and exponential smoothing. The target is removed from the scene below a threshold because recoloring left a silhouette. Part 1 used a single 0.1 Hz sine, part 2 a sum of five sines. Divisors 10 and 100 changed together with the trajectory, so the two parts cannot be compared directly. A flashlight variant (`archive/flashlight_visibility`) was tried and dropped.

## Stage 2, supervisor feedback

- Use the power law V = (a|v_q|)^n and tune a, no divisor.
- Threshold gives V = 1 or 0.
- Average the magnitudes of the FFTs rather than averaging in the time domain.
- Remove delays, since a debounce timer is nonlinear.
- Randomise the phases of the sines.

A pilot at n = 2, 4 and 6 followed, with hysteresis thresholds of 0.50 and 0.20, a smoothing weight and a tuned a. Tracking error was compared by ANOVA (`analysis/matlab/pilot`). The older PC introduced timing jitter that smeared the higher-frequency peaks, which led to the interpolation option in the loader. A faster PC was used for the main study.

## Stage 3, main study

Three conditions, ten trials each, six participants. The scene was rebuilt from the lab's original program structure with the new visibility rules, random phases, automatic file naming and a 60 s limit. Analysis moved from spectra and remnant power to system identification, linearity checks and the MMD test.

## Stage 4, analysis development

1. Mean spectra, first with coherent then with incoherent averaging.
2. Remnant power with the stimulus frequencies removed.
3. State-space model of the baseline, residual of the other conditions.
4. Transfer function T(s), poles, Bode plot, linearity check.
5. MMD permutation test using the authors' code.
6. Experiment 2 (error-only feedback) code written with the same visibility rule.
