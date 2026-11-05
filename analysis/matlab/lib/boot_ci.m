function [lo, hi] = boot_ci(Xc, nBoot, alpha)
% BOOT_CI  Percentile bootstrap interval of the incoherent mean spectrum.
%
%   [lo, hi] = boot_ci(Xc, nBoot, alpha)
%
%   Xc     [nTrials x nBins] complex one-sided spectra (see onesided_fft)
%   nBoot  resamples, for example 10000
%   alpha  0.05 gives a 95 percent interval
%
%   Whole trials are resampled with replacement. For every resample the
%   magnitudes are averaged (abs first, then mean). The interval is read
%   from the sorted resamples, so no toolbox is needed.

n  = size(Xc, 1);
bm = zeros(nBoot, size(Xc, 2));
for b = 1:nBoot
    idx = randi(n, n, 1);
    bm(b, :) = mean(abs(Xc(idx, :)), 1);
end
bm = sort(bm, 1);
lo = bm(max(1,     round(alpha/2     * nBoot)), :);
hi = bm(min(nBoot, round((1-alpha/2) * nBoot)), :);
end
