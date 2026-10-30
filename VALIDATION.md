# Validation — Prospero Hi-Fi v1.0.0

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
