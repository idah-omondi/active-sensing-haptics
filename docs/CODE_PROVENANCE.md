# Code provenance and licences

## Starting points

- The QuickHaptics scene structure of `haptics/exp1_target_tracking` follows the visual tracking demo written by Aysegul Kilic (Electrical and Electronics Engineering, METU, June 2023). That program is not redistributed here. The visibility logic, logging, random phases and condition handling are new.
- The analysis follows the approach of Karagoz et al. 2025: one-sided spectra, bootstrap intervals, state estimation view of active sensing, and the MMD permutation test. Their code and data are released under CC BY-SA 4.0 (https://aperta.ulakbim.gov.tr/records/274062). Nothing from that archive is included. `s06_mmd_test.m` calls its `mmd.m`, see `third_party/README.md`.
- OpenHaptics, QuickHaptics and the Touch X driver belong to 3D Systems and are not included.

## Authorship of this repository

Experiment design, scene modifications, data collection and analysis by Idah Omondi, with the supervision and feedback of the research group. The code was developed with the help of an AI assistant (Claude) in a long working session and consolidated into this layout.

## Consolidation notes

- `haptics/exp1_target_tracking/main.cpp` is one source for all three conditions. During the study the conditions were run from separate copies with the same logic. The visibility rule, the constants listed in `haptics/README.md`, the draw order and the CSV format are the same as in those copies. File names are built from `PARTICIPANT_ID`, the condition code and the trial number.
- `haptics/exp2_error_only_feedback/main.cpp` is the last Experiment 2 version, with only comments and the header changed.
- The MATLAB scripts are the study's scripts reorganised into functions with one configuration file. The loader, spectrum, bootstrap and remnant-power functions keep their original behaviour.
- The Experiment 1 pilot and the flashlight variant under `archive/` are cleaned versions of the earlier programs.

## Images

`docs/images/` holds photographs of the bench and exported analysis figures from the study. The photographs have had their metadata removed. Figures that the scripts regenerate (`figures/`) are not duplicated there.
