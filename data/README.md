# Data

Raw trial files are not committed. Put them here when running the analysis.

    data/
      raw/      main study, <stem><condition code>_<trial>.csv
      pilot/    earlier pilot, <condition>_<trial>.csv

Main study naming. Condition code 1 is baseline, 2 is threshold and 3 is continuous. Trial index runs 0 to 9. For participant stem `P1`:

    P11_0.csv ... P11_9.csv    baseline
    P12_0.csv ... P12_9.csv    threshold
    P13_0.csv ... P13_9.csv    continuous

Each file is one 60 s trial. Columns, logged at about 100 Hz:

| Column | Meaning | Unit |
|---|---|---|
| `Time(s)` | accumulated servo time | s |
| `TargetPosition` | target x | mm |
| `TargetVelocity` | finite difference of target position | mm/s |
| `HumanPosition` | stylus x | mm |
| `HumanVelocity` | stylus x velocity from the device | mm/s |
| `RelativePosition` | target minus stylus | mm |
| `RelativeVelocity` | target velocity minus stylus velocity, `v_q` | mm/s |
| `Visibility` | 1 in baseline, 0 or 1 in threshold, V = (a\|v_q\|)^n in continuous | none |

`Phase_Log.csv` holds one row per trial with the participant stem, the condition, the trial index and the five random phases in radians.

Experiment 2 files have these columns instead: `Time(s)`, `ErrorMagnitude`, `RawHumanVelocity`, `SmoothedHumanVelocity`, `VisibilityScore`, `VisibilityState`, `TargetPos`, `HumanPos`.

The data come from volunteers, so they stay out of the repository. To analyse participants under other file stems without committing them, copy `analysis/matlab/participants_local.example.m` to `participants_local.m`.
