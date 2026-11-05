function cfg = config_study()
% CONFIG_STUDY  Single place for paths, file naming and analysis constants.
%
%   cfg = config_study()
%
%   Raw CSVs live in data/raw/ and are named  <stem><condition code>_<trial>.csv
%   with condition code 1 = baseline, 2 = threshold, 3 = continuous and
%   trial 0..9. A participant is identified by the file stem, for example
%   stem 'P1' gives P11_0.csv ... P11_9.csv, P12_0.csv ... P13_9.csv.
%
%   Participant stems default to P1..P6. To use your own stems without
%   committing them, create analysis/matlab/participants_local.m (ignored by
%   git), see participants_local.example.m.

here = fileparts(mfilename('fullpath'));

% ---- Paths ---------------------------------------------------------------
cfg.root       = fullfile(here, '..', '..');
cfg.rawDir     = fullfile(cfg.root, 'data', 'raw');
cfg.figDir     = fullfile(cfg.root, 'figures');
cfg.resultsDir = fullfile(cfg.root, 'results');
cfg.thirdParty = fullfile(cfg.root, 'third_party');

% ---- Study design --------------------------------------------------------
cfg.conditions = {'baseline', 'threshold', 'continuous'};
cfg.condCode   = struct('baseline', 1, 'threshold', 2, 'continuous', 3);
cfg.nTrials    = 10;
cfg.filePattern = '%s%d_%d.csv';       % stem, condition code, trial index

cfg.participants = {'P1', 'P2', 'P3', 'P4', 'P5', 'P6'};
if isfile(fullfile(here, 'participants_local.m'))
    cfg.participants = participants_local();
end

% ---- Signal constants ----------------------------------------------------
cfg.Ts          = 0.01;      % s, logger period (10 ms)
cfg.Fs          = 1 / cfg.Ts;
cfg.sliceStart  = 1001;      % samples used by the spectral analysis (10 s to 50 s)
cfg.sliceEnd    = 5000;      % 4000 samples = 40 s, an integer number of cycles of every target frequency
cfg.useInterpolation = false; % true resamples onto an exact 10 ms grid using the logged timestamps

% ---- Stimulus ------------------------------------------------------------
% Sum of five sines. The base setting 36 is 0.1 Hz and the other components
% sit at 2.5, 5.5, 8.5 and 10 times the base frequency.
cfg.stimHz = [0.1 0.25 0.55 0.85 1.0];

% ---- Remnant power -------------------------------------------------------
cfg.remnant.tolerance = 0.03;   % Hz, half width of the band removed around each stimulus frequency
cfg.remnant.maxFreq   = 2.0;    % Hz, upper limit of the band that is summed

% ---- Residual spectrum ---------------------------------------------------
cfg.residual.tolerance = 0.05;  % Hz, mask half width in the residual figure and its remnant power
cfg.sysid.order        = 2;     % order of the state-space model (ssest)

% ---- Bootstrap -----------------------------------------------------------
cfg.nBoot = 10000;
cfg.alpha = 0.05;               % 95 percent interval
cfg.nShowBins = 61;             % frequency bins drawn in the mean FFT figure

% ---- MMD permutation test (Karagoz et al. 2025 settings) -----------------
cfg.mmd.sig     = 1;
cfg.mmd.shuff   = 10000;
cfg.mmd.pertest = 1;

if ~exist(cfg.figDir, 'dir'),     mkdir(cfg.figDir);     end
if ~exist(cfg.resultsDir, 'dir'), mkdir(cfg.resultsDir); end
end
