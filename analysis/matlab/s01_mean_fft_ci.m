function s01_mean_fft_ci(stem)
% S01_MEAN_FFT_CI  Mean spectrum of the human and the target in all three conditions.
%
%   s01_mean_fft_ci(stem)
%
%   For every condition: one-sided FFT of each trial, abs of each trial first,
%   then the mean over trials, with a 95 percent bootstrap interval. The target
%   is drawn once because it is the same stimulus in all conditions. Blue is
%   baseline, red is threshold and green is continuous.
%   Saved as figures/<stem>_mean_fft_ci.png.

cfg = config_study();
addpath(fullfile(fileparts(mfilename('fullpath')), 'lib'));
if nargin < 1, stem = cfg.participants{1}; end

human_col  = {[0 130 200]/255, [230 25 75]/255, [60 180 75]/255};
target_col = [245 130 48]/255;

R = struct();
for c = 1:numel(cfg.conditions)
    [human_x, target_x, params] = load_active_data(cfg, stem, cfg.conditions{c});
    L    = size(human_x, 2);
    half = floor(L/2) + 1;

    Human_c  = onesided_fft(human_x);
    Target_c = onesided_fft(target_x);

    R(c).label       = params.label;
    R(c).freqaxis    = (1/params.Ts) * (0:half-1) / L;
    R(c).human_mean  = mean(abs(Human_c),  1);
    R(c).target_mean = mean(abs(Target_c), 1);
    [R(c).human_lo, R(c).human_hi] = boot_ci(Human_c, cfg.nBoot, cfg.alpha);
    fprintf('Processed %s, %s\n', stem, params.label);
end

idx = 2:min(cfg.nShowBins, numel(R(1).freqaxis));   % drop the DC bin
fx  = R(1).freqaxis(idx);

fig = figure('Renderer', 'painters', 'Color', 'w', 'Units', 'centimeters');
hold on;
hTarget = plot(fx, R(1).target_mean(idx), 'Color', target_col, 'LineWidth', 2);
hHuman  = gobjects(1, numel(R));
txt     = cell(1, numel(R));
for c = 1:numel(R)
    patch([fx fliplr(fx)], [R(c).human_lo(idx) fliplr(R(c).human_hi(idx))], ...
          human_col{c}, 'FaceAlpha', 0.15, 'EdgeColor', 'none', 'HandleVisibility', 'off');
    hHuman(c) = plot(fx, R(c).human_mean(idx), 'Color', human_col{c}, 'LineWidth', 2);
    txt{c}    = ['human - ' R(c).label];
end
hCI = patch(NaN, NaN, [0.4 0.4 0.4], 'FaceAlpha', 0.15, 'EdgeColor', 'none');
hold off; box on;
xlim([fx(1) fx(end)]);
ax = gca; ax.FontSize = 7;
legend([hTarget hHuman hCI], [{'target'}, txt, {'95% bootstrap CI'}], ...
       'FontSize', 7, 'FontName', 'Helvetica', 'Location', 'best');
legend boxoff;
xlabel('frequency [Hz]', 'FontSize', 8, 'FontName', 'Helvetica');
ylabel('magnitude [mm]', 'FontSize', 8, 'FontName', 'Helvetica');
set(fig, 'Position', [10 10 15 10]);
save_fig(fig, cfg, sprintf('%s_mean_fft_ci', stem));
end
