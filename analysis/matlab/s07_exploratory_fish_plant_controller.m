function C = s07_exploratory_fish_plant_controller(stem)
% S07_EXPLORATORY_FISH_PLANT_CONTROLLER  Controller C(s) under a borrowed plant. Exploratory only.
%
%   C = s07_exploratory_fish_plant_controller(stem)
%
%   With unity feedback the closed loop is T = G/(1+G), so the forward path is
%   G = T/(1-T). If the plant P(s) were known, the controller would be C = G/P.
%   The plant of the Touch X stylus plus the human arm is not known. This script
%   substitutes the mass-spring-damper of the weakly electric fish from the
%   Karagoz et al. 2025 code
%        P(s) = k / (m s^2 + b s),   m = 0.0025, b = 0.0269, k = 0.2385
%   so the result describes a hypothetical controller for that plant, not a
%   measurement of the human controller. The resulting C(s) has a root in the
%   right half plane, which is a symptom of the mismatch.
%   Do not report it as the participant's controller.

cfg = config_study();
addpath(fullfile(fileparts(mfilename('fullpath')), 'lib'));
if nargin < 1, stem = cfg.participants{1}; end

[hb, tb, params] = load_active_data(cfg, stem, 'baseline');
T = tf(tfest(make_iddata(tb, hb, params.Ts), 2, 1));

m = 0.0025;  b = 0.0269;  k = 0.2385;
P = tf(k, [m, b, 0]);

G = minreal(T / (1 - T));
C = minreal(G / P);
disp('C(s) under the fish plant'); disp(C);
fprintf('Poles of C(s):\n'); disp(pole(C));
end
