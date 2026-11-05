# Haptics sources

Win32 applications for a Touch X using OpenHaptics 3.5.0 QuickHaptics, built with Visual Studio.

| Project | Purpose |
|---|---|
| `exp1_target_tracking/` | Main experiment. Baseline, threshold and continuous visibility in one source |
| `exp2_error_only_feedback/` | Compensatory task. The target is hidden and a bar shows the tracking error |
| `archive/pilot_velocity_divisor/` | First pilot, visibility = \|v_q\| divided by a constant, with deadband and smoothing |
| `archive/flashlight_visibility/` | Variant where visibility depends on distance to the target |
| `sandbox_ghost_object/` | Stage 1 static sphere, stage 2 sphere whose force fades while the stylus rests |

## Project setup

1. Install the Touch X driver and OpenHaptics 3.5.0, and confirm the device in Touch Setup.
2. Create an empty Windows Desktop Application project and add one `main.cpp`.
3. Project properties for all configurations
   - C/C++, Additional Include Directories: the OpenHaptics `include` and `utilities\include` folders and the QuickHaptics include folder.
   - Linker, Additional Library Directories: the OpenHaptics `lib` and `utilities\lib` folders.
   - Linker, Input: `hd.lib hl.lib hdu.lib hlu.lib` and the QuickHaptics library, plus the OpenGL and GLUT libraries that the install lists.
   - Linker, System, SubSystem: Windows. The programs use `WinMain`.

Folder names depend on where OpenHaptics was installed.

## Experiment 1, running a trial

Edit the three constants at the top of `exp1_target_tracking/main.cpp`.

| Constant | Values |
|---|---|
| `CONDITION` | `BASELINE`, `THRESHOLD`, `CONTINUOUS` |
| `PARTICIPANT_ID` | file stem, for example `"P1"` |
| `CURRENT_TRIAL` | 0 to 9 |

The output is `<PARTICIPANT_ID><condition code>_<trial>.csv` (1 baseline, 2 threshold, 3 continuous), so a trial never overwrites another. Each run also appends its five random phases to `Phase_Log.csv`. Logging stops at 60 s and the console prints `TRIAL COMPLETE`.

Visibility settings used

| Setting | Threshold | Continuous |
|---|---|---|
| exponent n | 2 | 2 |
| a | 0.005 | 0.006 |
| smoothing of \|v_q\| | EMA, weight 0.05 | none |
| hysteresis | on at V above 0.50, off at V below 0.20 | none |
| target drawn as | solid or moved off screen | gray level 1 - V |

Stimulus: `frequency = 36` (0.1 Hz base), `part = 2`, `B = 20` mm per sine.

## Threads

QuickHaptics runs two threads. The servo thread (`computeForceCB`, about 1 kHz) computes the target, velocities, visibility and the log rows, and every 10th tick is written to the CSV. The graphics thread (`GraphicsCallback`) draws the boxes from values the servo thread shares through globals.

## Design choices that matter

- When the target is hidden it is moved off screen. Recoloring to white leaves a silhouette where the target overlaps the subject's box.
- Threshold uses a hysteresis gap and a low-pass filter on the velocity. A single threshold made the target flicker from velocity noise, and a debounce timer was rejected because a sample-and-hold is nonlinear and leaks into the spectrum.
- Phases are random per trial so the path cannot be memorised.

Build messages and fixes are in [`../docs/BUILD_TROUBLESHOOTING.md`](../docs/BUILD_TROUBLESHOOTING.md).
