function [p, tbl, stats] = pilot_rmse_anova(folder)
% PILOT_RMSE_ANOVA  Tracking error across the exponent conditions of the pilot.
%
%   [p, tbl, stats] = pilot_rmse_anova(folder)
%
%   Pilot with the threshold rule at exponents n = 2, 4 and 6 plus a baseline.
%   Files are <condition>_<0..9>.csv in folder, with conditions Baseline2,
%   Tracking2, Tracking4 and Tracking6. Ten seconds are trimmed from each end
%   of every trial. RMSE of (target - human) is computed per trial, then a
%   one-way ANOVA and a multiple comparison are run on the per-trial values.
%   Trials come from one participant, so they are not independent samples of a
%   population, and the p value should be read with that in mind.

if nargin < 1
    here = fileparts(mfilename('fullpath'));
    folder = fullfile(here, '..', '..', '..', 'data', 'pilot');
end
outDir = fullfile(fileparts(mfilename('fullpath')), '..', '..', '..', 'results');
if ~exist(outDir, 'dir'), mkdir(outDir); end

conds = {'Baseline2', 'Tracking2', 'Tracking4', 'Tracking6'};
trim  = 10;     % s at each end
rmse  = nan(10, numel(conds));

for c = 1:numel(conds)
    for k = 0:9
        T = readtable(fullfile(folder, sprintf('%s_%d.csv', conds{c}, k)), 'VariableNamingRule', 'preserve');
        t = T.('Time(s)');
        keep = t >= (t(1) + trim) & t <= (t(end) - trim);
        e = T.TargetPosition(keep) - T.HumanPosition(keep);
        rmse(k+1, c) = sqrt(mean(e .^ 2, 'omitnan'));
    end
end

[p, tbl, stats] = anova1(rmse, conds, 'off');
fprintf('One-way ANOVA on RMSE: F = %.3f, p = %.3g\n', tbl{2,5}, p);

figure('Color', 'w'); boxplot(rmse, 'Labels', conds);
ylabel('RMSE (mm)'); grid on; title('Pilot tracking error by condition');
figure('Color', 'w'); multcompare(stats);
writetable(array2table(rmse, 'VariableNames', conds), fullfile(outDir, 'pilot_rmse_values.csv'));
end
