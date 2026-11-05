function [T, T_first] = s05_closed_loop_tf(stem)
% S05_CLOSED_LOOP_TF  Closed-loop transfer function of baseline tracking and the linearity check.
%
%   [T, T_first] = s05_closed_loop_tf(stem)
%
%   T        tfest(2 poles, 1 zero) on all ten baseline trials, Y(s)/X(s)
%   T_first  the same estimate from the first baseline trial only
%
%   Prints the transfer function, its poles, and the gain and phase at the five
%   stimulus frequencies. Then three figures
%     _bode_T                 Bode plot of T(s)
%     _linearity_baseline     baseline human spectrum against target spectrum
%                             with the stimulus frequencies marked
%     _predicted_vs_actual    first threshold trial, spectrum predicted by the
%                             baseline linear model (T_first) against the
%                             spectrum the participant produced
%
%   Linearity rule used. A linear system can only output frequencies that were
%   put in. Power between the stimulus lines means the response is not linear.

cfg = config_study();
addpath(fullfile(fileparts(mfilename('fullpath')), 'lib'));
if nargin < 1, stem = cfg.participants{1}; end

[hb, tb, params] = load_active_data(cfg, stem, 'baseline');
[hh, th, ~]      = load_active_data(cfg, stem, 'threshold');
Ts = params.Ts;  Fs = 1/Ts;
L  = size(hb, 2);
f  = Fs * (0:floor(L/2)) / L;
time_vec = (0:L-1)' * Ts;

% ---- Transfer function ---------------------------------------------------
T = tf(tfest(make_iddata(tb, hb, Ts), 2, 1));
fprintf('T(s) = Y(s)/X(s), all baseline trials\n'); disp(T);
fprintf('Poles:\n'); disp(pole(T));

T_first = tf(tfest(make_iddata(tb(1, :), hb(1, :), Ts), 2, 1));

% ---- Gain and phase at the stimulus frequencies ---------------------------
[mag, phase] = bode(T, cfg.stimHz * 2 * pi);
mag = squeeze(mag);  phase = squeeze(phase);
for k = 1:numel(cfg.stimHz)
    fprintf('Freq %.2f Hz | Gain %.2f | Phase %.2f deg\n', cfg.stimHz(k), mag(k), phase(k));
end

fig = figure('Color', 'w', 'Position', [100 100 800 600]);
bode(T); grid on;
title(sprintf('Bode plot, closed-loop human tracking T(s), %s', stem));
save_fig(fig, cfg, sprintf('%s_bode_T', stem));

% ---- Linearity check on baseline ------------------------------------------
Ph = zeros(size(hb, 1), numel(f));  Pt = Ph;
for i = 1:size(hb, 1)
    Ph(i, :) = abs(onesided_fft(hb(i, :)));
    Pt(i, :) = abs(onesided_fft(tb(i, :)));
end
fig = figure('Color', 'w', 'Position', [100 100 800 500]);
plot(f, mean(Ph, 1), 'b', 'LineWidth', 2); hold on;
plot(f, mean(Pt, 1), 'r--', 'LineWidth', 2);
for k = 1:numel(cfg.stimHz), xline(cfg.stimHz(k), 'k:', 'LineWidth', 1.5); end
xlim([0 2.5]); grid on;
xlabel('Frequency (Hz)'); ylabel('Magnitude (mm)');
title(sprintf('Baseline linearity check, %s', stem));
legend('Human output', 'Target input', 'Stimulus frequencies', 'Location', 'best');
save_fig(fig, cfg, sprintf('%s_linearity_baseline', stem));

% ---- Predicted linear output against what the participant did --------------
y_pred   = lsim(T_first, th(1, :)', time_vec);
y_actual = hh(1, :)';
fig = figure('Color', 'w', 'Position', [100 100 800 400]);
plot(f, abs(onesided_fft(y_pred')),   'k--', 'LineWidth', 2); hold on;
plot(f, abs(onesided_fft(y_actual')), 'r',   'LineWidth', 1.5);
for k = 1:numel(cfg.stimHz), xline(cfg.stimHz(k), 'k:'); end
xlim([0 2.5]); grid on;
xlabel('Frequency (Hz)'); ylabel('Magnitude (mm)');
title(sprintf('Predicted linear output against actual output, threshold trial 1, %s', stem));
legend('Predicted from baseline model', 'Actual human movement', 'Stimulus frequencies', 'Location', 'best');
save_fig(fig, cfg, sprintf('%s_predicted_vs_actual', stem));
end
