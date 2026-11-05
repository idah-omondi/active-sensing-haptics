% RUN_ALL  Pipeline for every participant, then the group summary.
%   >> run_all
cfg = config_study();
for p = 1:numel(cfg.participants)
    try
        run_participant(cfg.participants{p});
    catch err
        fprintf('Skipped %s: %s\n', cfg.participants{p}, err.message);
    end
end
s08_group_remnant_stats();
