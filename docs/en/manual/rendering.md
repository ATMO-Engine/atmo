---
id: rendering
title: Rendering platforms
order: 2
description: Which GPU API the engine draws with on each OS, how it is chosen, and what each one needs.
---

# Rendering platforms

The [[RenderingServer]] draws through a [[RenderingPlatform]], one per GPU API. The engine ships three:

| Platform | Built by default on | Purpose                                                   |
| -------- | ------------------- | --------------------------------------------------------- |
| Metal    | macOS               | The macOS backend                                         |
| Vulkan   | Windows, Linux      | The Windows and Linux backend, opt-in on macOS (MoltenVK) |
| Null     | every OS            | Draws nothing; used by `--headless`                       |

There is no Direct3D 12 platform: Vulkan is the Windows backend. D3D12 is planned for later.

## How a platform is chosen {#selection}

The engine picks the best platform for the machine it runs on:

| OS      | Tried in order      |
| ------- | ------------------- |
| macOS   | Metal, Vulkan, Null |
| Windows | Vulkan, Null        |
| Linux   | Vulkan, Null        |

A platform is skipped when it is not built or when the GPU lacks what it needs. A platform that only finds a
software rasterizer (such as lavapipe) is used only if no other platform has a real GPU. If no GPU platform can
start outside headless mode, the engine shows an error dialog and quits instead of opening an empty window.

`--rendering-platform=metal|vulkan|null` asks for a specific platform; if it is unavailable, the engine falls back
to the automatic choice.

```sh
xmake run atmo --rendering-platform=vulkan --gpu-validation
```

Rendering runs on its own thread. `--render-thread=off` runs it on the main thread, for debugging.

## Metal {#metal}

- Needs macOS 13 or later and a GPU in the Metal 3 family with tier 2 argument buffers: every Apple silicon Mac
  and the recent AMD and Intel GPUs. Older Macs fall back to Vulkan when it is built, otherwise to Null.
- Built by default on macOS; `xmake f --rendering_metal=n` turns it off. It uses
  [metal-cpp](https://developer.apple.com/metal/cpp/), so it is plain C++ with no Objective-C.
- Shaders are embedded as Metal source and compiled by the system when the engine starts.
- On macOS 15 and later, resources are kept resident with a residency set; on macOS 13 and 14 they are declared on
  every render pass instead. `ATMO_METAL_RESIDENCY_SETS=0` forces the macOS 13 path, for testing.

## Vulkan {#vulkan}

- Needs a Vulkan 1.3 driver with dynamic rendering, synchronization2 and descriptor indexing. On Windows, the
  driver from the GPU vendor (NVIDIA, AMD, Intel, Qualcomm) provides it; the generic Microsoft display driver used
  by some virtual machines and remote sessions has no Vulkan.
- Built by default on Windows and Linux; `xmake f --rendering_vulkan=n` builds without it.
- Opt-in on macOS, because it needs MoltenVK (a long first build that requires Xcode) and Metal is the macOS
  backend. Run `xmake f --rendering_vulkan=y` once, then start the engine with `--rendering-platform=vulkan`. The
  build copies `libMoltenVK.dylib` next to the binary. A Vulkan loader installed on the system (for example from
  the Vulkan SDK) is used first, which is what makes `--gpu-validation` work.

::: warning
Do not commit the `xmake-requires.lock` changes that `--rendering_vulkan=y` causes on macOS: the lock file matches
the default configuration of each platform.
:::

## Pipeline cache {#pipeline-cache}

Compiled pipelines are cached in `~/Library/Application Support/Atmo/Atmo/pipeline-cache/` on macOS. Vulkan keeps
its cache in the same folder (`vulkan-pipelines.bin`) and discards it when the GPU or driver changes. The folder
can be deleted at any time.

## GPU validation {#gpu-validation}

`--gpu-validation` turns on the platform's validation layer:

- **Metal** — API and shader validation (`MTL_DEBUG_LAYER`, `MTL_SHADER_VALIDATION`). Frames can be captured
  with Xcode's GPU debugger.
- **Vulkan** — needs the Khronos validation layer (`VK_LAYER_KHRONOS_validation`) to be installed.
