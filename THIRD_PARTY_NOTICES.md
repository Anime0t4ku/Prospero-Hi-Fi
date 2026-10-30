# Prospero Hi-Fi — third-party notices

Prospero Hi-Fi is a GPL-3.0 derivative of Anime0t4ku's NX Hi-Fi v1.1.0.

- PS5SX2 (Swordpdf / swordpdf and Heyde Moura): PS5 disc transport in ps5/disc; keyboard page arrangement, function-key widths and vertical navigation adapted from AchievementAccountPanel and BuildPanelKeyboard. GPL-3.0-or-later. Source: https://github.com/Swordpdf/PS5SX2
- Native title converter, startup and generated runtime module: Copyright 2026 BlackBearReloaded, GPL-3.0-or-later; reproduced from public PS5SX2 and Mihawk-99/PS5_Vulkan source trees. The generated libc.prx is a compatibility companion; it contains no Sony implementation code. Its source generator and metadata manifests are included. https://github.com/mihawk-99/PS5_Vulkan
- PS5 SDL2 port: John Törnblom and SDL contributors, zlib license. Full source is in third_party/SDL-ps5. Altered files are marked for Prospero: active-resolution tracking, buffer allocation, app-owned keyboard. https://github.com/ps5-payload-dev/SDL
- PS5 payload SDK: John Törnblom and contributors, GPL-3.0-or-later with BSD-licensed FreeBSD headers and Apache/LLVM emulated TLS code. https://github.com/ps5-payload-dev/sdk
- PacBrew port libraries: https://github.com/ps5-payload-dev/pacbrew-repo (pinned v0.41.1). SDL2_image/SDL2_ttf (zlib), FreeType (FTL/GPL), HarfBuzz (MIT), libpng (libpng), libjpeg-turbo (BSD/IJG/zlib), libwebp (BSD), libsamplerate (BSD), libiconv (LGPL), libcurl (curl), OpenSSL (Apache 2.0), libpsl (MIT), zstd (BSD/GPL), libsmb2 (LGPL), bzip2 (BSD), zlib (zlib), LLVM libc++/libc++abi/libunwind (Apache 2.0 with LLVM exception). Existing port library license texts are in licenses/portlibs.
- miniaudio and stb_vorbis: retain their bundled public-domain/MIT notices.
- nlohmann/json: MIT, bundled notice retained.
- Noto Sans CJK JP: SIL Open Font License 1.1, bundled as assets/font.ttf (OpenType data); source https://github.com/notofonts/noto-cjk. Japanese and other CJK metadata glyphs are supported. DejaVu Sans is used only when generating the launcher artwork; its license is retained.
- CA certificates: system Mozilla-derived trust bundle; certificate data is bundled for verified HTTPS, with certificate verification enabled.

All original attribution headers remain in the source. No game files, BIOS, proprietary Sony runtime, or console keys are included.
