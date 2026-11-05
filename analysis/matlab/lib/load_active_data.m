function [human_x, target_x, params] = load_active_data(cfg, stem, condition)
% LOAD_ACTIVE_DATA  Stack the trials of one participant and condition.
%
%   [human_x, target_x, params] = load_active_data(cfg, stem, condition)
%
%   cfg        output of config_study
%   stem       participant file stem, for example 'P1'
%   condition  'baseline', 'threshold' or 'continuous'
%
%   human_x    [nTrials x 4000] stylus position, mm
%   target_x   [nTrials x 4000] target position, mm
%   params     struct with Ts (s), label, stem, files
%
%   Each trial is cut to samples 1001..5000 of the 60 s recording, so the
%   analysed window is 40 s, which holds a whole number of cycles of 0.1,
%   0.25, 0.55, 0.85 and 1.0 Hz. NaN samples are set to 0. With
%   cfg.useInterpolation the window is resampled onto an exact 10 ms grid
%   from the logged timestamps, otherwise the rows are used as logged.

nTrials = cfg.nTrials;
nSamp   = cfg.sliceEnd - cfg.sliceStart + 1;
human_x  = zeros(nTrials, nSamp);
target_x = zeros(nTrials, nSamp);
files = cell(1, nTrials);

code = cfg.condCode.(condition);

for i = 1:nTrials
    fname = sprintf(cfg.filePattern, stem, code, i - 1);
    path  = fullfile(cfg.rawDir, fname);
    if ~isfile(path)
        error('load_active_data:missingFile', 'File not found: %s', path);
    end
    files{i} = fname;

    data = readmatrix(path);
    raw_time   = data(:, 1);
    raw_target = data(:, 2);
    raw_human  = data(:, 4);
    raw_target(isnan(raw_target)) = 0;
    raw_human(isnan(raw_human))   = 0;

    if numel(raw_human) < cfg.sliceEnd
        error('load_active_data:tooShort', ...
            '%s has %d samples, need at least %d.', fname, numel(raw_human), cfg.sliceEnd);
    end

    idx = cfg.sliceStart:cfg.sliceEnd;
    if cfg.useInterpolation
        t0 = raw_time(idx(1));
        tq = t0 + (0:nSamp-1) * cfg.Ts;
        target_x(i, :) = interp1(raw_time(idx), raw_target(idx), tq, 'linear');
        human_x(i, :)  = interp1(raw_time(idx), raw_human(idx),  tq, 'linear');
    else
        target_x(i, :) = raw_target(idx)';
        human_x(i, :)  = raw_human(idx)';
    end
end

params.Ts    = cfg.Ts;
params.label = condition;
params.stem  = stem;
params.files = files;
end
