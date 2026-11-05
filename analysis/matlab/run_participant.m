function run_participant(stem)
% RUN_PARTICIPANT  Run the full pipeline for one participant.
%
%   run_participant('P1')
%
%   Order follows the order the analysis was developed in
%     1  mean spectra with bootstrap intervals
%     2  remnant power
%     3  baseline state-space model
%     4  residual spectrum of the other conditions
%     5  closed-loop transfer function and linearity check
%     6  MMD permutation test (only if mmd.m is available)

cfg = config_study();
if nargin < 1, stem = cfg.participants{1}; end

s01_mean_fft_ci(stem);
s02_remnant_power(stem);
s03_baseline_sysid(stem);
s04_residual_fft(stem);
s05_closed_loop_tf(stem);

if exist('mmd', 'file') || isfile(fullfile(cfg.thirdParty, 'mmd.m'))
    s06_mmd_test(stem);
else
    fprintf('Skipping MMD test, mmd.m not in third_party/.\n');
end
end
