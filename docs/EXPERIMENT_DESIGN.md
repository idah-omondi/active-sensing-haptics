# Experiment design

## Idea

In the electrosensory model of the weakly electric fish (Karagoz et al. 2025) the measurement is

    y = a * p_q * v_q + b * v_q + rho

where p_q and v_q are the relative position and velocity between animal and object. If v_q is zero the measurement carries no information about p_q. The animal has to keep moving relative to the object to keep it observable, and it does so more when its signal degrades. This is active sensing.

The human version makes the same thing true on a screen. The target is visible according to the relative velocity v_q = (target velocity) - (stylus velocity). A participant who matches the target's velocity perfectly makes v_q zero, and the target vanishes. To see it again they have to move relative to it.

## Setup

- Touch X haptic device, OpenHaptics 3.5.0 QuickHaptics, Visual Studio. No force is rendered, the stylus is only a position and velocity sensor.
- Scene structure from the lab's visual tracking demo (A. Kilic, METU, 2023). The visibility logic is new.
- A gray box (target) and an orange box (the subject's stylus) move along x. The stylus box is always solid.
- The target is a sum of five sines at 0.1, 0.25, 0.55, 0.85 and 1.0 Hz, 20 mm per component. These are 1, 2.5, 5.5, 8.5 and 10 times a 0.1 Hz base (`frequency = 36` degrees per second).
- A random phase is drawn for each sine on every run, uniform on [0, 2 pi), and written to `Phase_Log.csv`.
- Trials last 60 s and are logged at about 100 Hz. Ten trials per condition.
- Six participants. Each session was about 40 minutes of the same tracking task.

## Why random phases

If all five sines start at the same phase their peaks line up at the start and the path becomes easy to predict from trial to trial. Random phases give a different smooth path every trial, so the participant cannot memorise it. The generator is uniform, so no phase is favoured.

A consequence for the analysis. The target spectrum has the same magnitudes in every trial but different phases, so complex spectra must not be averaged across trials. The analysis takes the magnitude of each trial first and averages afterwards. Averaging first cancelled peaks and removed the 1.0 Hz component from the mean target spectrum in the first analysis.

## Conditions

| Condition | Visibility | Target on screen |
|---|---|---|
| Baseline | V = 1 | always visible |
| Threshold | V = (a * s)^n with s the low-pass filtered \|v_q\|, then a hysteresis gate | solid gray or removed |
| Continuous | V = (a * \|v_q\|)^n | gray level 1 - V, fading into the white background |

Parameters of the main study. n = 2. Threshold uses a = 0.005, an exponential moving average with weight 0.05, turn-on at V above 0.50 and turn-off at V below 0.20. Continuous uses a = 0.006 and no smoothing. The visibility equation was chosen with the supervisor, with no division by a constant and with a tuned a. The threshold gives a binary 1 or 0.

## Visibility details

- Hysteresis stopped the target from blinking. A single threshold flickered because the device velocity is noisy and a human hand speeds up and slows down constantly.
- A debounce timer was rejected, a sample-and-hold is nonlinear and injects low-frequency power into the spectrum.
- Heavy smoothing of the logged signals was avoided. Smoothing is applied only inside the visibility rule. The logged velocities are the raw device values.
- A hidden target is moved to (10000, 10000, 0). Recoloring to white left a silhouette where the target overlapped the stylus box.
- Draw order. Threshold and baseline draw the target in front of the stylus box when visible, continuous draws the stylus box in front.

## Experiment 2, error-only feedback

A compensatory version. The target is not drawn. A teal bar shows the tracking error (target minus stylus) and the participant keeps it short. The bar is shown or hidden by Score = a * v^n with a single threshold, where v is the participant's own exponentially smoothed hand speed. This version was built and logged to its own CSV format, see `data/README.md`.

## Pilot stages

The history of how the design arrived here is in [`PROJECT_HISTORY.md`](PROJECT_HISTORY.md).
