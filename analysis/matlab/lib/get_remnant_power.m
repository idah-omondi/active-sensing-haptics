function remnant_power = get_remnant_power(filename, Fs, target_freqs, tolerance, max_freq)
% GET_REMNANT_POWER  Spectral power that is not at the stimulus frequencies.
%
%   remnant_power = get_remnant_power(filename, Fs, target_freqs, tolerance, max_freq)
%
%   Reads the human position (column 4) of the whole recording, takes the
%   one-sided magnitude spectrum, keeps bins above 0 Hz up to max_freq, deletes
%   a band of +/- tolerance around every stimulus frequency, and sums the
%   squared magnitudes of what is left. A missing file returns NaN.

if ~isfile(filename)
    warning('get_remnant_power:missingFile', '%s not found. Returning NaN.', filename);
    remnant_power = NaN;
    return;
end

data  = readmatrix(filename);
human = data(:, 4);
human(isnan(human)) = 0;

L = length(human);
Y = fft(human) / L;
Y_onesided = abs(Y(1:floor(L/2) + 1));
Y_onesided(2:end-1) = 2 * Y_onesided(2:end-1);
freqs = Fs * (0:floor(L/2)) / L;

valid_idx   = (freqs > 0) & (freqs <= max_freq);
f_valid     = freqs(valid_idx);
power_valid = Y_onesided(valid_idx);

mask = true(size(f_valid));
for k = 1:numel(target_freqs)
    band = (f_valid >= target_freqs(k) - tolerance) & (f_valid <= target_freqs(k) + tolerance);
    mask(band) = false;
end

remnant_power = sum(power_valid(mask) .^ 2);
end
