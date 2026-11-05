function T = s02_remnant_power(stem)
% S02_REMNANT_POWER  Power away from the stimulus frequencies, per trial and condition.
%
%   T = s02_remnant_power(stem)
%
%   The five stimulus frequencies are masked out (+/- 0.03 Hz), and the squared
%   magnitudes of every other bin up to 2 Hz are summed. This is the energy
%   the target did not ask for. Writes results/<stem>_remnant_power.csv and
%   figures/<stem>_remnant_boxplot.png, and prints the condition means.

cfg = config_study();
addpath(fullfile(fileparts(mfilename('fullpath')), 'lib'));
if nargin < 1, stem = cfg.participants{1}; end

n = cfg.nTrials;
scores = nan(n, numel(cfg.conditions));
for c = 1:numel(cfg.conditions)
    code = cfg.condCode.(cfg.conditions{c});
    for i = 0:n-1
        f = fullfile(cfg.rawDir, sprintf(cfg.filePattern, stem, code, i));
        scores(i+1, c) = get_remnant_power(f, cfg.Fs, cfg.stimHz, ...
            cfg.remnant.tolerance, cfg.remnant.maxFreq);
    end
end

T = table((1:n)', scores(:,1), scores(:,3), scores(:,2), ...
    'VariableNames', {'Trial', 'Baseline', 'Continuous', 'Threshold'});
disp(T);
writetable(T, fullfile(cfg.resultsDir, sprintf('%s_remnant_power.csv', stem)));

fprintf('\nMean remnant power for %s\n', stem);
fprintf('  Baseline   %.2f\n', mean(T.Baseline,   'omitnan'));
fprintf('  Continuous %.2f\n', mean(T.Continuous, 'omitnan'));
fprintf('  Threshold  %.2f\n', mean(T.Threshold,  'omitnan'));

fig = figure('Color', 'w');
boxplot([T.Baseline, T.Continuous, T.Threshold], ...
    'Labels', {'Baseline', 'Continuous', 'Threshold'});
ylabel('Remnant power'); grid on;
title(sprintf('Remnant power by condition, %s', stem));
save_fig(fig, cfg, sprintf('%s_remnant_boxplot', stem));
end
