function [sys1, sysAll] = s03_baseline_sysid(stem)
% S03_BASELINE_SYSID  State-space model of normal tracking from baseline trials.
%
%   [sys1, sysAll] = s03_baseline_sysid(stem)
%
%   Baseline trials are the "lights on" case, with the target always visible.
%   The target position is the input and the stylus position the output. A
%   2nd-order state-space model is estimated with ssest from
%     sys1    the first baseline trial only
%     sysAll  all ten baseline trials merged (the universal model)
%   The fit percentages from compare and the A and B matrices are printed.
%   Saved as figures/<stem>_sysid_fit_single.png and _fit_all.png.

cfg = config_study();
addpath(fullfile(fileparts(mfilename('fullpath')), 'lib'));
if nargin < 1, stem = cfg.participants{1}; end

[human, target, params] = load_active_data(cfg, stem, 'baseline');

d1   = make_iddata(target(1, :), human(1, :), params.Ts);
dAll = make_iddata(target, human, params.Ts);

fprintf('Estimating order %d state-space model, trial 1\n', cfg.sysid.order);
sys1 = ssest(d1, cfg.sysid.order);
fprintf('Estimating order %d state-space model, all %d trials\n', cfg.sysid.order, cfg.nTrials);
sysAll = ssest(dAll, cfg.sysid.order);

[~, fit1]   = compare(d1,   sys1);
[~, fitAll] = compare(dAll, sysAll);
fprintf('Fit, single trial: %.2f %%\n', mean(fit1));
fprintf('Fit, all trials:   %.2f %%\n', mean(fitAll));

disp('Single-trial model A, B'); disp(sys1.A); disp(sys1.B);
disp('Universal model A, B');    disp(sysAll.A); disp(sysAll.B);

fig1 = figure('Color', 'w'); compare(d1, sys1);
title(sprintf('Model vs actual, baseline trial 1, %s', stem));
save_fig(fig1, cfg, sprintf('%s_sysid_fit_single', stem));

fig2 = figure('Color', 'w'); compare(dAll, sysAll);
title(sprintf('Model vs actual, all baseline trials, %s', stem));
save_fig(fig2, cfg, sprintf('%s_sysid_fit_all', stem));
end
