# Prospero Hi-Fi AGC renderer

The complete SDL interface uses the native `ps5agc` accelerated backend on PS5.
It submits textured and coloured triangles through the system AGC API, using
two scanout buffers, GPU completion markers and wait-until-safe packets. No
OpenGL software renderer or CPU framebuffer upload is used. Consecutive
compatible geometry is batched, including the visualizer's line quads.

The 36-byte vertex layout and native setup are adapted from Rufidj/ps5link-sdk's
GPU cube and SvenGDK/SharpProspero. The mesh vertex shader is reused with
identity matrices: the normal's x/y carry texture coordinates into TEXCOORD0,
and the packed colour reaches TEXCOORD1. The textured pixel shader samples
RGBA and multiplies it by the interpolated colour, including alpha.

Shader sources, containers and the container packer are included here. To
regenerate the pixel program with an LLVM build supporting gfx1030:

    python3 ps5/gpu/agcpack.py texture-container ps5/gpu/pixel-template.sb /tmp/pixel-template.sb
    llvm-mc -triple=amdgcn-amd-amdhsa -mcpu=gfx1030 -filetype=obj ps5/gpu/textured_p.s -o /tmp/pixel.o
    llvm-objcopy --dump-section .text=/tmp/pixel.bin /tmp/pixel.o
    python3 ps5/gpu/agcpack.py pack /tmp/pixel-template.sb /tmp/pixel.bin ps5/gpu/textured_ps.sb --vgprs 4
    python3 ps5/gpu/agcpack.py array third_party/SDL-ps5/src/render/ps5agc/textured_ps_sb.h textured_ps_sb ps5/gpu/textured_ps.sb
    python3 ps5/gpu/agcpack.py array third_party/SDL-ps5/src/render/ps5agc/mesh_vs_sb.h mesh_vs_sb ps5/gpu/mesh_vs.sb

The renderer advertises only the operations implemented: no render-to-texture,
custom blend equations or GPU readback. The app does not need those operations.
Output resolution is selected from the console's current mode at launch; restart
the app after changing HDMI resolution. Live scanout reallocation is disabled
for the GPU backend to avoid releasing memory still owned by the GPU.

## Sources and licensing

- https://github.com/Rufidj/ps5link-sdk — GPL-3.0, renderer setup, pixel assembly and shader packer.
- https://github.com/SvenGDK/SharpProspero — GPL-3.0, vertex container and PSSL source, native descriptors.
- https://github.com/mihawk-99/PS5_Vulkan — GPL-3.0-or-later, synchronization and blend-state reference.

This integration needs PS5 hardware validation. A successful build and mock API
checks do not establish correct pixels or 60 FPS on a console. Performance data
is written to `/data/prospero-hifi/performance.json`; `renderer: ps5agc` and
`gpu_accelerated: true` identify the selected backend.
