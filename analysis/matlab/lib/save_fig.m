function save_fig(fig, cfg, name)
% SAVE_FIG  Save a figure as PNG in figures/ at 300 dpi.
if ~exist(cfg.figDir, 'dir'), mkdir(cfg.figDir); end
exportgraphics(fig, fullfile(cfg.figDir, [name '.png']), 'Resolution', 300);
end
