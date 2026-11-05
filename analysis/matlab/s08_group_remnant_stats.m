function s08_group_remnant_stats()
% S08_GROUP_REMNANT_STATS  Remnant power across participants.
%
%   s08_group_remnant_stats()
%
%   Takes the mean remnant power of every participant in each condition and
%   shows the three conditions side by side, one line per participant. The test
%   is the Friedman test, the nonparametric counterpart of a repeated-measures
%   ANOVA, because with a handful of participants normality cannot be checked.
%   Kendall's W is reported as the effect size. Participants with missing files
%   are skipped.
%
%   Needs the Statistics and Machine Learning Toolbox (friedman).

cfg = config_study();
addpath(fullfile(fileparts(mfilename('fullpath')), 'lib'));

M = []; used = {};
for p = 1:numel(cfg.participants)
    stem = cfg.participants{p};
    row = nan(1, 3);
    try
        for c = 1:3
            code = cfg.condCode.(cfg.conditions{c});
            v = nan(cfg.nTrials, 1);
            for i = 0:cfg.nTrials-1
                v(i+1) = get_remnant_power(fullfile(cfg.rawDir, sprintf(cfg.filePattern, stem, code, i)), ...
                    cfg.Fs, cfg.stimHz, cfg.remnant.tolerance, cfg.remnant.maxFreq);
            end
            row(c) = mean(v, 'omitnan');
        end
    catch
        row(:) = NaN;
    end
    if all(~isnan(row)), M(end+1, :) = row; used{end+1} = stem; end %#ok<AGROW>
end

if size(M, 1) < 3
    error('s08:notEnough', 'Need at least 3 participants with complete data, found %d.', size(M, 1));
end

[p, ~, st] = friedman(M, 1, 'off');
k = size(M, 1);  m = size(M, 2);
W = 12 * sum((st.meanranks * k - k*(m+1)/2) .^ 2) / (k^2 * m * (m^2 - 1));
fprintf('Friedman test across %d participants: p = %.4g, Kendall W = %.2f\n', k, p, W);

T = array2table(M, 'VariableNames', {'Baseline', 'Threshold', 'Continuous'}, 'RowNames', used);
disp(T);
writetable(T, fullfile(cfg.resultsDir, 'group_remnant_power.csv'), 'WriteRowNames', true);

fig = figure('Color', 'w'); hold on;
plot(1:3, M', '-o', 'Color', [0.6 0.6 0.6]);
plot(1:3, mean(M, 1), '-s', 'Color', 'k', 'LineWidth', 2.5, 'MarkerFaceColor', 'k');
xticks(1:3); xticklabels({'Baseline', 'Threshold', 'Continuous'}); xlim([0.7 3.3]);
ylabel('Mean remnant power'); grid on;
title(sprintf('Remnant power by condition, %d participants', k));
save_fig(fig, cfg, 'group_remnant_power');
end
