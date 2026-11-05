function [testStat, pvalue] = s06_mmd_test(stem, cond_a, cond_b)
% S06_MMD_TEST  Maximum mean discrepancy permutation test between two conditions.
%
%   [testStat, pvalue] = s06_mmd_test(stem)                      baseline vs threshold
%   [testStat, pvalue] = s06_mmd_test(stem, 'baseline', 'continuous')
%
%   Compares the distribution of trial spectra (ten one-sided magnitude spectra
%   per condition) with the kernel two-sample test used in Karagoz et al. 2025,
%   Appendix B.2 and B.3: Gaussian kernel with sigma 1, 10000 shuffles.
%
%   Needs mmd.m and rbf_dot.m from the authors' dataset, see
%   third_party/README.md.
%
%   Reading the result. The statistic depends on the kernel width and on the
%   scale of the spectra, so its size means little on its own. The p value is
%   the fraction of shuffles that gave a statistic at least as large as the
%   observed one. With 10 trials against 10 trials it cannot be smaller than
%   about 1/10000, so a printed 0 means p < 1e-4.

cfg = config_study();
addpath(fullfile(fileparts(mfilename('fullpath')), 'lib'));
addpath(genpath(cfg.thirdParty));
if nargin < 1, stem = cfg.participants{1}; end
if nargin < 2, cond_a = 'baseline';  end
if nargin < 3, cond_b = 'threshold'; end

if ~exist('mmd', 'file')
    error('s06_mmd_test:missingMMD', ...
        'mmd.m not found. Put mmd.m and rbf_dot.m in third_party/ (see third_party/README.md).');
end

ha = load_active_data(cfg, stem, cond_a);
hb = load_active_data(cfg, stem, cond_b);

A = abs(onesided_fft(ha));
B = abs(onesided_fft(hb));

prs.sig     = cfg.mmd.sig;
prs.shuff   = cfg.mmd.shuff;
prs.pertest = cfg.mmd.pertest;

fprintf('MMD permutation test, %s vs %s, %s, %d shuffles\n', cond_a, cond_b, stem, prs.shuff);
[testStat, pvalue] = mmd(A, B, prs);

fprintf('MMD test statistic: %.4e\n', testStat);
if pvalue == 0
    fprintf('p value: < %.0e (no shuffle reached the observed statistic)\n', 1/prs.shuff);
else
    fprintf('p value: %.4g\n', pvalue);
end
end
