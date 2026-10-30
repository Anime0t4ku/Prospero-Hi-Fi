# Validation — Prospero Hi-Fi v1.1.0

## Completed locally

- Cross-compiled the native x86-64 PS5 executable with the public SDK and PacBrew libraries. Native module conversion and FSELF signing completed successfully.
- Compiled the app-owned keyboard, PS5 CAM disc backend and one-shot storage-access helper.
- Rebuilt the loader companion from its included generator. `sce_module/libc.prx` matches upstream SHA-256 `e6ff45d16adf687855cc3b33b0c8a4132b6504360b221e0a34c7e99fb3ba0036`.
- All nine host regression suites passed: core/config/metadata, disc TOC/MusicBrainz/cache/CDDA input, layout, socket backpressure, concurrent SMB lifecycle (24,000 sessions), simulated CDDA player, bounded cover decoding, incoming HTTP and NX-compatible WebSocket client integration.
- Player tests exercise gapless timing, pause/seek, repeat, track advance and read-error handling with simulated CD sectors.
- HTTP tests verify the unchanged NX protocol identity, Prospero display name, Screen Control capability, rejected outgoing discovery/probe, browsing, playback dispatch, source revisions, credential redaction and validation.
- Built and ran the full app with Linux SDL dummy video/audio to inspect the actual interface at 1920x1080 and 3840x2160. Fonts and UI scale with the framebuffer. This is a host rendering check, not proof of console video output.
- Checked the ShadowMount+ title ID/content ID, required files, PNG dimensions, FSELF signatures, asset checksums and archive integrity.

## Required console verification

No physical PS5 was available in this build environment. Verify on 13.60/Y2JB/ShadowMount+ before treating this as a stable release:

1. Home-screen launch, runtime loading, local ELF-loader helper and persistent storage access.
2. DualSense input/reconnection and every keyboard page, masked passwords and cancellation.
3. HDMI audio, seek accuracy, EQ, shuffle/repeat and sustained local/USB/SMB playback.
4. Actual user-selected 720p/1080p/1440p/2160p output, hot changes and rendering performance.
5. External USB optical-drive enumeration, audio CD TOC/sector access, gapless playback, mixed-mode discs, swaps/removal and MusicBrainz/artwork access.
6. Browser control, NX independent control, NX Screen Control and remote text entry over the LAN.
7. Older firmware and PS5 hardware models. SDK-supported versions are candidates, not a tested compatibility list.

The first build uses CPU SDL rendering with tiled VideoOut presentation. It includes refreshed graphics but no Vulkan/OpenGL shader pipeline or background playback while another game runs.

## v0.1.1 follow-up validation

- A focused host benchmark of the same 3840x2160 background/panel pass measured 107.54 ms/frame before and 14.60 ms/frame after the rendering changes (25 frames, software SDL dummy output). This is a host comparison, not a PS5 measurement or end-to-end controller latency claim.
- Input-repeat regression tests pass for immediate presses, the same repeat delay under different frame cadences, quick reversals and recovery after a stalled frame.
- Native PS5 compilation/signing and folder verification repeated for v0.1.1.
- Square 512x512 launcher PNG verified fully opaque, including all corners.
- Previous nine regression suites passed for v0.1.0; this follow-up changes rendering/input timing and adds diagnostics. Console testing is still required.

## v0.1.2 validation

- All twelve host regression suites passed: core, disc metadata/input, layout, button repeat, controller keyboard, PS5 network-address API simulation, socket writes, SMB lifecycle, CDDA player, cover codec, HTTP remote and WebSocket client.
- Keyboard tests exercise Square UTF-8 deletion, Triangle space, L1/R1 page changes, Cross typing, Options acceptance and Circle cancellation without committing edits.
- Network tests inject valid LAN addresses, invalid/zero/loopback/broadcast addresses and an API failure. They verify use of the PS5 address selector; a live console address query still needs testing.
- Native PS5 compile, module conversion and signing completed. Package and ZIP integrity checks passed.
- Full app inspected with host SDL output at 1080p and 4K. The artwork shadow and enlarged artwork background are removed; the gradient is generated at output resolution. Native framebuffer size is retained.
- A warmed 4K host menu/text draw-pass comparison (50 frames, SDL software/dummy video) measured 19.08 ms/frame for v0.1.1 and 10.92 ms/frame for v0.1.2. This benchmark is not a console FPS or controller latency measurement.
- Console presentation now relies on the driver's display-flip wait instead of adding a second frame sleep. Remaining console input latency and menu frame rate need user testing.
- Performance diagnostics identify menu/fullscreen view in addition to timing data.

## v1.0.0 storage source validation

- All thirteen C++ host regression suites passed, including the new storage suite, existing keyboard/network tests, audio/CDDA, cover decoding, HTTP and NX-compatible WebSocket tests.
- Storage tests cover internal-only visibility, M.2 plus multiple USB volumes, duplicate/unknown mount handling, optical presence without requiring a TOC, stable source IDs, Music defaults, independent folder persistence, invalid/cross-drive paths, removal and migration of the old combined USB folder.
- Embedded browser JavaScript parses; source-card tests cover source visibility, opening the correct drive, saving a folder without losing another drive's setting, and the disc action.
- Device scans run off the UI thread. The native build checks mounts with statfs and directory access; it requires USB CAM transport plus optical INQUIRY identity for disc visibility and playback selection. It never requires an inserted CD to show Physical Disc.
- Native compile/module conversion/signing, package validation and ZIP integrity checks completed. Actual USB bridge and firmware behaviour require console verification.
- Full host app HTTP state verified version 1.0.0, Internal Storage visibility and absent-drive/disc filtering. The actual menu was inspected at 1080p.
- Existing NX independent-control clients retain their original source schema. Additional detected drives are available through the updated browser and NX Screen Control.

## Corrected v1.0.0 launch build

- Console report: v1.0.0 aborts with 0xa002030a / SYSTEM_ILLEGAL_FUNCTION_CALL. Disassembly confirmed the added statfs function executed a raw syscall (number 396) from the app text. This is a likely cause; no console stack trace was available.
- Removed statfs and its payload-only mount structure use from drive detection. Detection compares stat device IDs against the parent filesystem and checks directory access. This hides leftover directories while avoiding the new raw syscall.
- Native executable is checked for statfs/fstatfs/getfsstat symbols and syscall instructions originating in the new device-scan code. Existing storage, browser and regression tests remain applicable. Console relaunch verification is required.

- Focused storage tests also reject empty and populated leftover directories on the parent's filesystem and recognise a real host mount. Folder persistence/migration and browser-source tests pass after the fix.
- Version remains v1.0.0 as requested; replace the complete previous title folder with this corrected build.

## v1.1.0 player update

- All fourteen C++ host regression suites passed, including audio/CDDA, cover handling, storage detection, HTTP/NX-compatible WebSocket and settings tests. Browser-source JavaScript checks passed.
- Player focus/navigation tests cover separate menu/fullscreen rows, action mappings, bounded selection, pointer-hit geometry, Auto Fullscreen persistence/type validation and 100% defaults while preserving an existing saved 80% volume.
- Host app integration exercises fullscreen L1/R1/Touchpad isolation, its in-player EQ preset dialog, Circle exit, seek/volume/transport focus and Auto Fullscreen on/off with a real WAV file. Subsequent track changes leave browsing open.
- Host rendering checks inspect the actual menu/fullscreen layouts at 1080p and 4K. Screen Control retains transport-icon drawing operations and uses the same pointer geometry.
- Native PS5 build/signing, folder validation and complete ready-folder ZIP integrity are checked. No additional raw mount syscalls are introduced.
- Controller ergonomics and audio behaviour of this v1.1.0 build still need PS5 testing.

## v1.1.1 interface update

- Compact controls-only footer uses vector PS5 button symbols. The expanded menu panels display thirteen rows in the PS5 logical layout; option-dialog row counts are unchanged.
- Playback icons are coverage-sampled and cached at the actual output scale. Screen Control receives matching icon scanline operations.
- Metadata headings and wrapped codec-badge rows are centered; badge text is centered horizontally and vertically using font metrics.
- Updated app integration checks Play/Pause focus on fullscreen entry, absence of the old footer explanation and fullscreen button labels, and centered badge text. Fullscreen isolation, seeking/volume navigation, EQ dialog and Auto Fullscreen behaviour are retained.
- Native build/signing, host regression checks, actual host rendering at 1080p/4K, package verification and archive checks are performed. PS5 testing remains required.

## v1.1.2 alignment correction

- Restored the v1.1.0 left alignment for track title, artist, album and technical badge rows. Technical badge text remains centered inside each badge; the new large fullscreen metadata panel is removed.
- Total duration is measured in renderer coordinates and ends at the seek bar's right endpoint in both players. This covers known, estimated and unknown duration labels.
- App integration verifies title alignment, duration endpoints and the absence of the large fullscreen metadata panel while retaining badge text centering and player navigation tests.
- Native build/package checks and host rendering checks are performed. Console verification remains required.

## Final v1.1.0 release adjustments

The 1.1.1/1.1.2 headings above describe development iterations; this work ships as the next release, v1.1.0.

- Natural sorting compares digit runs without integer conversion, retains folders first and deterministic case handling, and supports long numbers and leading zeroes.
- Fullscreen L1/R1 and L2/R2 share previous/next actions; pressing both previous/next equivalents together advances only once. Main-menu L1/R1 retain tab switching.
- Badge layout centers cropped visible glyph bounds directly inside the rendered box, accounting for font side bearings and coordinate scaling. Pixel tests check the rendered result at multiple resolutions.

- Final validation: all sixteen C++ host regression suites passed, along with the browser-source JavaScript check and application player integration tests. Pixel checks confirm balanced visible badge margins at 1080p, 1440p and 4K. The source patch is cumulative over v1.0.0 and includes all recent UI corrections.

## v1.1.0 Settings categories and browser redraw

- Settings now has a left category column, middle entry column and the existing right mini player. Cross enters a category; Circle returns to categories. Each category remembers its entry selection, including through mini-player focus changes. Category prefixes are removed from entry labels. Pointer/Screen Control hit targets match the new columns; the independent remote settings schema is unchanged.
- Every frame resets clipping and explicitly clears the previous frame before painting the background. Each list row has a scoped clip to keep filename/selection rendering inside its own bounds. This targets the reported filename ghosting/flicker after returning from mini-player focus; the original PS5-only symptom has not been reproduced on desktop, so console confirmation remains required.
- Seventeen C++ host regression suites cover all settings fields, category/entry navigation, remembered positions and the existing audio/storage/network behaviour. App integration checks actual Cross/Circle/Touchpad actions, choice dismissal, field editing and browser redraw recovery over 90 frames with playback active, focus switching, scrolling and an intentionally stale clip/poisoned previous frame. The rendered browser matches the clean reference in every comparison.
- Native PS5 compilation/signing and package integrity checks pass. Settings is visually inspected at 1080p; existing badge pixel checks cover 1080p, 1440p and 4K. Version remains v1.1.0. No console test has been performed for this update.
