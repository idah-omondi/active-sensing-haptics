# Build and runtime troubleshooting

| Symptom | Cause and fix |
|---|---|
| IntelliSense E1097 about `no_init_all` | Harmless editor message. Ignore it. |
| Warning C4273 on `qhStart`, inconsistent dll linkage | Harmless with the QuickHaptics headers. Ignore it. |
| `Sphere(radius)` has no matching constructor | The 3.5.0 header only has `Sphere(radius, resolution)` and `Sphere(radius, resolution, x, y, z)`. Use `Sphere(15.0, 20.0)`. |
| LNK2019 unresolved external `_main` | The project is set to the Console subsystem. Set Linker, System, SubSystem to Windows. The code uses `WinMain`. |
| No console output | Windows subsystem hides the console. Call `AllocConsole()` and redirect with `freopen_s`, as `stage2_decay` does. |
| Console looks frozen | Output is buffered after `freopen_s`. Call `fflush(stdout)` after `printf`. |
| LNK2019 on `getWindowParameters` | Declared but not exported by the library. Do not call it. Resize the window manually. |
| C2374 redefinition of `visibility` | A merge left two declarations. Keep one. |
| Device blocked, stylus will not move freely | Check the device is not in the inkwell, calibrate in Touch Setup, check cable and power. Rule out the application by running `stage1_static_sphere` first. |
| CSV overwritten | Experiment 1 names files from `PARTICIPANT_ID`, condition and `CURRENT_TRIAL`. Change `CURRENT_TRIAL` for every run. The archived pilot used a fixed `LOG_FILENAME`. |
| C2664 or E0167, `const char*` to `char*` | QuickHaptics expects `char*`. Cast string literals, for example `(char*)"devCursor"`. |
| C2664 on `hduStartRecord` | `recordCallback` must not carry the `HLCALLBACK` tag, which changes the calling convention. |
| Target jumps to the center for a frame | Resetting a position and setting a new one in two calls lets the renderer draw in between. Compute the position once and set it in one call where possible. |
| Higher-frequency FFT peaks smeared | Logged timestamps are not exactly 10 ms apart. Set `cfg.useInterpolation = true`. |
