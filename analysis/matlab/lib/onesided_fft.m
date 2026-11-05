function Xc = onesided_fft(x)
% ONESIDED_FFT  One-sided, amplitude-scaled FFT, one trial per row.
%
%   Xc = onesided_fft(x)
%
%   Returns the complex spectrum [nTrials x L/2+1]. The scaling is 1/L, and
%   bins 2..end-1 are doubled, so a sine of amplitude a gives a peak of a.
%   Take abs() of every trial first and average afterwards. Averaging the
%   complex spectra first cancels components whose phase differs between
%   trials, which is exactly what the random target phases cause.

L  = size(x, 2);
X  = fft(x, [], 2) / L;
Xc = X(:, 1:floor(L/2) + 1);
Xc(:, 2:end-1) = 2 * Xc(:, 2:end-1);
end
