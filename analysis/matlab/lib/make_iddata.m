function d = make_iddata(target_x, human_x, Ts)
% MAKE_IDDATA  Package trials as one multi-experiment iddata object.
%
%   d = make_iddata(target_x, human_x, Ts)
%
%   Input is the target position, output is the stylus position. The trials
%   are merged as separate experiments, so the estimator never joins the end
%   of one trial to the start of the next.

d = iddata(human_x(1, :)', target_x(1, :)', Ts);
for i = 2:size(human_x, 1)
    d = merge(d, iddata(human_x(i, :)', target_x(i, :)', Ts));
end
d.InputName  = 'Target X';
d.OutputName = 'Human Y';
end
