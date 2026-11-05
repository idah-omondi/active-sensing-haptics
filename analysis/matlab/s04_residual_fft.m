function s04_residual_fft(stem)
% S04_RESIDUAL_FFT  What the baseline model cannot explain in the other conditions.
%
%   s04_residual_fft(stem)
%
%   1. Fit the universal baseline model (all ten baseline trials, ssest).
%   2. Feed the target of every threshold and continuous trial through that
%      model with lsim. This is the stylus movement the participant would have
%      produced if they had tracked as they did with the target fully visible.
%   3. residual = actual stylus position - predicted stylus position.
%   4. One-sided spectrum of every residual, mean over trials.
%   5. Delete a band around each of the five stimulus frequencies. Anything
%      that stands out in the gaps was not on the screen.
%
%   The mean residual spectrum is drawn with the stimulus bands masked, and the
%   residual remnant power is printed per condition.
%   Saved as figures/<stem>_residual_fft_masked.png.

cfg = config_study();
addpath(fullfile(fileparts(mfilename('fullpath')), 'lib'));
if nargin < 1, stem = cfg.participants{1}; end

[hb, tb, params] = load_active_data(cfg, stem, 'baseline');
Ts = params.Ts;
sys = ssest(make_iddata(tb, hb, Ts), cfg.sysid.order);

L = size(hb, 2);
t = (0:L-1)' * Ts;
f = (1/Ts) * (0:floor(L/2)) / L;

mask = true(size(f));
for k = 1:numel(cfg.stimHz)
    mask(abs(f - cfg.stimHz(k)) < cfg.residual.tolerance) = false;
end

conds = {'threshold', 'continuous'};
col   = {[230 25 75]/255, [60 180 75]/255};
meanRes = zeros(numel(conds), numel(f));

for c = 1:numel(conds)
    [h, tg, ~] = load_active_data(cfg, stem, conds{c});
    res = zeros(size(h, 1), numel(f));
    for i = 1:size(h, 1)
        y_sim    = lsim(sys, tg(i, :)', t);
        residual = h(i, :)' - y_sim;
        res(i, :) = abs(onesided_fft(residual'));
    end
    meanRes(c, :) = mean(res, 1);
    rp = sum(meanRes(c, mask) .^ 2);
    fprintf('%s residual remnant power (tolerance %.2f Hz): %.4f\n', ...
        conds{c}, cfg.residual.tolerance, rp);
end

fig = figure('Color', 'w', 'Position', [100 100 800 450]); hold on;
for c = 1:numel(conds)
    y = meanRes(c, :);
    y(~mask) = NaN;                 % physical gaps where the stimuli were
    plot(f, y, 'Color', col{c}, 'LineWidth', 2);
end
xlim([0 2.5]); grid on;
xlabel('Frequency (Hz)'); ylabel('Magnitude (mm)');
title(sprintf('Mean residual spectrum with stimulus frequencies removed, %s', stem));
legend(conds, 'Location', 'best');
save_fig(fig, cfg, sprintf('%s_residual_fft_masked', stem));
end
