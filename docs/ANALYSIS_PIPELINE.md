# Analysis pipeline

Every step is a script in `analysis/matlab/`. Constants are in `config_study.m`.

## 0. Loading

`load_active_data` reads the trial CSVs of one participant and condition. It takes samples 1001 to 5000 of each 60 s recording, so a 40 s window from 10 s to 50 s. 40 s holds a whole number of cycles of every stimulus frequency (4 cycles of 0.1 Hz, 10 of 0.25, 22 of 0.55, 34 of 0.85, 40 of 1.0), so every component falls on one FFT bin. Columns used are time (1), target position (2) and human position (4). NaN samples are set to 0.

Timing. The logger runs on every 10th servo tick, and the time axis is accumulated from the instantaneous update rate, so rows are close to but not exactly 10 ms apart. On the older, laggy PC this smeared the 0.85 Hz and 1.0 Hz peaks and gave fractional cycle counts. Linear interpolation of the logged timestamps onto an exact 10 ms grid fixed it. On the final PC the rows are used as logged and `cfg.useInterpolation` switches the interpolation on.

## 1. Spectra (`s01_mean_fft_ci`)

1. One-sided FFT of every trial, scaled by 1/L, interior bins doubled.
2. Magnitude of each trial.
3. Mean over trials.
4. 95 percent bootstrap interval, 10,000 resamples of whole trials with replacement, same order of operations.

The order matters because of the random phases, see `EXPERIMENT_DESIGN.md`.

## 2. Remnant power (`s02_remnant_power`)

The spectrum of each trial up to 2 Hz, with a band of 0.03 Hz on each side of each stimulus frequency deleted. The squared magnitudes of what is left are summed. This is the energy the target did not ask for. It is computed per trial, giving ten values per condition.

## 3. Baseline model (`s03_baseline_sysid`)

`ssest` of order 2 with the target position as input and the stylus position as output. Estimated from the first baseline trial alone and from all ten baseline trials merged as separate experiments (`merge`, so the end of one trial is never joined to the start of the next). The model is then a digital copy of how this participant tracks when the target is visible. System identification here means fitting model equations to input and output recordings so that the model reproduces the output.

## 4. Residual (`s04_residual_fft`)

The universal baseline model is run on the target of each threshold and continuous trial with `lsim`. The prediction is subtracted from the stylus position, giving a residual. The spectrum of the residual is averaged over trials and a band of 0.05 Hz around each stimulus frequency is removed. Gaps are drawn as breaks. Power that remains in the gaps cannot be explained by following the visible target. The residual remnant power is printed per condition.

Reaction time shifts between trials leave some of the normal tracking in the residual (leakage). That is the reason the stimulus bands are removed before reading the residual.

## 5. Closed-loop transfer function and linearity (`s05_closed_loop_tf`)

`tfest` with 2 poles and 1 zero gives T(s) = Y(s)/X(s), the ratio of stylus output to target input. It is not an average of the trials, the estimator finds the dynamics that best map all ten input recordings to their ten outputs. Poles, gain and phase at the five stimulus frequencies, and a Bode plot are produced.

The linearity rule. A linear system can only output frequencies that were put in. For baseline the human spectrum is plotted on the target spectrum with the stimulus frequencies marked. For threshold the first trial's target is fed through the baseline model and the predicted spectrum is plotted against the actual one. The predicted spectrum has power only on the stimulus lines, the actual spectrum has power between them.

## 6. MMD test (`s06_mmd_test`)

The maximum mean discrepancy two-sample test from the paper (Appendix B.2 and B.3). The ten magnitude spectra of one condition are compared with the ten of another, with a Gaussian kernel of sigma 1 and 10,000 shuffles. The p value is the fraction of shuffles that reach the observed statistic. The statistic itself depends on the kernel width and the scale of the spectra, so its size is not interpreted. A p value of 0 means p < 1e-4.

## 7. Exploratory controller (`s07_exploratory_fish_plant_controller`)

With unity feedback T = G/(1+G), so G = T/(1-T) and C = G/P for a plant P. The plant of the stylus and arm is unknown, so the fish plant of the paper is used as a stand-in. The result is a hypothetical controller, not a measurement.

## 8. Group (`s08_group_remnant_stats`)

Mean remnant power per participant in each condition. Friedman test across participants with Kendall's W as the effect size. Written for the full six-participant set.

## Next steps

- Extended Kalman filter simulation from the paper's `run_predictive_uncertainty` code, fed with this experiment's target, to test whether a predictive-uncertainty model produces the wiggle.
- A feature matrix (total remnant power, largest peak, velocity variance) with a gradient-boosting classifier of condition.
- Per-participant models for all six participants.
