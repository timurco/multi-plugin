# OFX Caching System - Deep Dive Analysis

## Introduction

This document provides a comprehensive analysis of the OFX (OpenFX) caching system, specifically focusing on how it works for temporal effects and simulations. The goal is to understand the OFX caching architecture so we can properly implement it with WebGPU for an Optical Flow fluid simulation plugin.

---

## Part 1: OFX Caching Architecture Overview

### What is "Caching" in OFX context?

There are **two completely different types of caching** in OFX:

#### 1. Plugin Metadata Cache (Not what we need)
- **Purpose**: Speed up plugin discovery across sessions
- **What it caches**: Plugin capabilities, parameters, clips
- **Location**: XML file on disk (`pluginCache.xml`)
- **Example**: `HostSupport/examples/cacheDemo.cpp`
- **Managed by**: Host application
- **Relevance to us**: ❌ Not relevant for simulation caching

#### 2. Render/Frame Cache (What we need!)
- **Purpose**: Store rendered frame results to avoid re-computation
- **What it caches**: Rendered pixel data, simulation state
- **Location**: Host memory (RAM/VRAM), sometimes disk
- **Managed by**: **Host, not plugin**
- **Relevance to us**: ✅ **Critical for our Optical Flow simulation**

---

## Part 2: How OFX Render Caching Works

### Key Principle: Host Controls the Cache

**IMPORTANT**: In OFX, the **host** (DaVinci Resolve, Nuke, etc.) manages the render cache, NOT the plugin.

```
┌─────────────────────────────────────────────┐
│             Host Application                │
│  ┌───────────────────────────────────────┐ │
│  │       Render Cache Manager            │ │
│  │                                       │ │
│  │  Frame 1: [cached pixel data]        │ │
│  │  Frame 2: [cached pixel data]        │ │
│  │  Frame N: [cached pixel data]        │ │
│  └───────────────────────────────────────┘ │
│             ↓                  ↑            │
│          render()         pixel output      │
│             ↓                  ↑            │
│  ┌───────────────────────────────────────┐ │
│  │         OFX Plugin Instance           │ │
│  │  (Your Optical Flow Effect)           │ │
│  └───────────────────────────────────────┘ │
└─────────────────────────────────────────────┘
```

### How the Host Decides When to Cache

The host looks at:

1. **Input dependencies** - declared via `kOfxImageEffectActionGetFramesNeeded`
2. **Parameter state** - did any parameters change?
3. **Time** - which frame is being requested?
4. **Render Quality** - draft vs final
5. **Memory pressure** - available RAM/VRAM

If inputs/params unchanged → **cache hit** → skip `render()` call
If something changed → **cache miss** → call `render()`

---

## Part 3: Temporal Dependencies - GetFramesNeeded Action

### Purpose

Tell the host which input frames are needed to render a given output frame.

### Action Definition

```cpp
kOfxImageEffectActionGetFramesNeeded
```

**When called**: Before rendering, when host needs to know dependencies

**In Args**:
- `kOfxPropTime` - the frame being queried

**Out Args**:
- `OfxImageClipPropFrameRange_<ClipName>` - array of frame ranges needed

### Example: Temporal Blur (needs previous 5 frames)

```cpp
OfxStatus getFramesNeeded(OfxImageEffectHandle effect,
                          OfxPropertySetHandle inArgs,
                          OfxPropertySetHandle outArgs)
{
    double time;
    gPropHost->propGetDouble(inArgs, kOfxPropTime, 0, &time);

    int currentFrame = (int)time;

    // We need 5 frames before current frame
    double rangeSource[2];
    rangeSource[0] = currentFrame - 5;  // start
    rangeSource[1] = currentFrame;       // end (inclusive)

    // Set the range for "Source" clip
    gPropHost->propSetDoubleN(outArgs,
        "OfxImageClipPropFrameRange_Source",
        2, rangeSource);

    return kOfxStatOK;
}
```

### Example: Discontinuous Ranges

```cpp
// Effect needs frame 0 AND previous frame
double ranges[4];
ranges[0] = 0;              // Always need first frame
ranges[1] = 0;
ranges[2] = currentFrame - 1;  // Also need previous
ranges[3] = currentFrame;

gPropHost->propSetDoubleN(outArgs,
    "OfxImageClipPropFrameRange_Source",
    4, ranges);  // 4 values = 2 ranges
```

### Why This Matters for Caching

When host knows frame dependencies, it can:
- ✅ Pre-fetch input frames from cache
- ✅ Determine if render is needed (cache invalidation)
- ✅ Optimize parallel rendering (independent frames can render concurrently)

---

## Part 4: Sequential Rendering

### What is Sequential Rendering?

A mode where the host guarantees frames will be rendered in strict temporal order on a single plugin instance.

### Properties

**Declare support**:
```cpp
// In describe() or describeInContext()
gPropHost->propSetInt(effectProps,
    kOfxImageEffectInstancePropSequentialRender,
    0,  // index
    2); // 2 = prefer sequential, 1 = support, 0 = no
```

Values:
- `0` - Plugin does NOT support sequential rendering
- `1` - Plugin CAN handle sequential rendering (optional)
- `2` - Plugin PREFERS sequential rendering (for simulations!)

**During render**:
```cpp
// In render() action, check if we're actually in sequential mode
int isSequential;
gPropHost->propGetInt(inArgs,
    kOfxImageEffectPropSequentialRenderStatus,
    0, &isSequential);

if (isSequential) {
    // We can rely on frames being rendered in order
    // Safe to maintain state between frames
}
```

### Why Sequential Matters for Simulations

For Optical Flow / Fluid simulations:
- Frame N **depends on** accumulated state from frames 0..N-1
- **Cannot** render frame 100 before frame 99
- Must process in strict order: 0 → 1 → 2 → ... → N

```cpp
// Simulation pseudocode
wgpu::Buffer accumulatedFlow = nullptr;

for (int frame = 0; frame < totalFrames; frame++) {
    // Compute optical flow for current frame
    wgpu::Buffer currentFlow = computeOpticalFlow(frame);

    // Accumulate with previous state
    accumulatedFlow = accumulate(accumulatedFlow, currentFlow);

    // Render using accumulated state
    render(accumulatedFlow);
}
```

---

## Part 5: Begin/End Sequence Render

### Actions

```cpp
kOfxImageEffectActionBeginSequenceRender
kOfxImageEffectActionEndSequenceRender
```

### Lifecycle

```
Host calls:
  ↓
beginSequenceRender(startFrame=0, endFrame=100, frameStep=1)
  ↓
render(frame=0)
  ↓
render(frame=1)
  ↓
...
  ↓
render(frame=100)
  ↓
endSequenceRender(startFrame=0, endFrame=100)
```

### In Args (BeginSequenceRender)

- `kOfxImageEffectPropFrameRange` - [start, end] frames
- `kOfxImageEffectPropFrameStep` - step (1.0 for progressive, 0.5 for fields)
- `kOfxPropIsInteractive` - is this user scrubbing or batch render?
- `kOfxImageEffectPropRenderScale` - scaling factor
- `kOfxImageEffectPropSequentialRenderStatus` - will it be sequential?

### Plugin Implementation

```cpp
class OpticalFlowEffect : public ImageEffect {
private:
    wgpu::Buffer _accumulatedFlow;  // Persistent state

public:
    OfxStatus beginSequenceRender(double startFrame,
                                   double endFrame,
                                   double frameStep,
                                   bool isInteractive,
                                   OfxPointD renderScale,
                                   bool isSequential,
                                   bool isInteractive)
    {
        // Initialize simulation state
        _accumulatedFlow = createEmptyFlowBuffer();

        return kOfxStatOK;
    }

    OfxStatus render(double time, ...)
    {
        // Use _accumulatedFlow from previous frame
        wgpu::Buffer currentFlow = computeOpticalFlow(time);
        _accumulatedFlow = accumulate(_accumulatedFlow, currentFlow);

        // Render with accumulated state
        // ...

        return kOfxStatOK;
    }

    OfxStatus endSequenceRender(...)
    {
        // Clean up
        _accumulatedFlow.Destroy();
        _accumulatedFlow = nullptr;

        return kOfxStatOK;
    }
};
```

---

## Part 6: Instance Data Persistence - SyncPrivateData

### The Problem

Sometimes a plugin has internal state that **cannot** be represented by OFX parameters alone.

Examples:
- Simulation cache (our case!)
- Lookup tables
- Compiled shaders
- Training data for ML effects

### The Solution: kOfxActionSyncPrivateData

When the host wants to save the project or duplicate an instance, it needs the plugin's internal state.

### Action

```cpp
kOfxActionSyncPrivateData
```

**When called**:
- Before saving project
- Before duplicating instance
- After loading project

**What plugin should do**:
- Serialize internal state to parameters (custom params)
- Or write to external file referenced by parameter

### Property to Check

```cpp
kOfxPropParamSetNeedsSyncing
```

Plugin sets this to `1` when internal state has changed.
Host checks it before save/copy operations.

### Implementation Example

```cpp
class OpticalFlowEffect : public ImageEffect {
private:
    struct SimulationCache {
        std::map<int, wgpu::Texture> frames;
        bool dirty = false;
    } _cache;

    StringParam* _cachePathParam;  // Hidden param storing cache file path

public:
    OfxStatus syncPrivateData() override
    {
        if (!_cache.dirty) {
            return kOfxStatOK;  // Nothing to sync
        }

        // 1. Generate unique cache file path
        std::string projectPath = getProjectPath();
        std::string cachePath = projectPath + "/.ofx_cache/optical_flow_" +
                                getInstanceID() + ".cache";

        // 2. Serialize cache to disk
        std::ofstream file(cachePath, std::ios::binary);
        for (auto& [frame, texture] : _cache.frames) {
            writeTextureToDisk(file, frame, texture);
        }
        file.close();

        // 3. Store path in hidden parameter
        _cachePathParam->setValue(cachePath);

        // 4. Mark as clean
        _cache.dirty = false;

        // 5. Tell host we're synced
        paramSetNeedsSyncing(false);

        return kOfxStatOK;
    }

    OfxStatus createInstance() override
    {
        // After loading project, restore cache from file
        std::string cachePath = _cachePathParam->getValue();

        if (std::filesystem::exists(cachePath)) {
            std::ifstream file(cachePath, std::ios::binary);
            loadCacheFromDisk(file);
            file.close();
        }

        return kOfxStatOK;
    }

    void render(double time, ...) override
    {
        // Compute frame...
        wgpu::Texture result = computeFrame(time);

        // Cache it
        _cache.frames[(int)time] = result;
        _cache.dirty = true;

        // Mark param set as needing sync
        paramSetNeedsSyncing(true);
    }
};
```

---

## Part 7: Cache Invalidation

### When Does Host Invalidate Cache?

1. **Parameter changed** - any param modification
2. **Input clip changed** - source footage changed
3. **GetFramesNeeded changed** - dependencies shifted
4. **Manual purge** - user clicked "Clear Cache"
5. **Memory pressure** - host needs to free memory

### How Plugin Triggers Invalidation

No direct API! Plugin cannot say "invalidate frame X".

Instead:
- Change a parameter → host auto-invalidates
- Use hidden "progress" parameter trick (from our previous conversation)

---

## Part 8: WebGPU Integration Strategy

### Challenges

1. **WebGPU resources live in VRAM**
   - Limited capacity (4-16 GB typical)
   - Host cache is in RAM
   - Need transfer between GPU ↔ CPU

2. **Host doesn't know about WebGPU**
   - Host expects CPU pixel buffers
   - Plugin must download from GPU for each render

3. **Simulation state is large**
   - 300 frames @ 1920×1080 RGBA32F = 14 GB
   - Cannot fit entirely in VRAM

### Solution: Hybrid Cache Architecture

```
┌────────────────────────────────────────────────────────┐
│                    Host Cache (RAM)                    │
│  Managed by Resolve/Nuke - plugin has no control      │
│  Stores final rendered frames                         │
└────────────────────────────────────────────────────────┘
                      ↑
                      │ copyTextureToOutput()
                      │
┌────────────────────────────────────────────────────────┐
│              Plugin WebGPU Cache (VRAM)                │
│                                                        │
│  GPU Hot Cache (LRU, 30 frames = 1.4 GB)             │
│  ┌──────────────────────────────────────┐            │
│  │ Frame 70: [wgpu::Texture] [Flow]     │            │
│  │ Frame 71: [wgpu::Texture] [Flow]     │            │
│  │ ...                                  │            │
│  │ Frame 100: [wgpu::Texture] [Flow]    │            │
│  └──────────────────────────────────────┘            │
│                      ↑                                │
│                      │ on cache miss                  │
│                      ↓                                │
│  CPU Compressed Cache (100 frames = 1.2 GB RAM)      │
│  ┌──────────────────────────────────────┐            │
│  │ Frame 1: [compressed buffer]         │            │
│  │ Frame 2: [compressed buffer]         │            │
│  │ ...                                  │            │
│  │ Frame 100: [compressed buffer]       │            │
│  └──────────────────────────────────────┘            │
│                      ↑                                │
│                      │ on cache miss                  │
│                      ↓                                │
│  Disk Cache (300 frames = 3.6 GB)                    │
│  ~/.ofx_cache/optical_flow_instanceID/               │
│  ├─ frame_0000.bin                                   │
│  ├─ frame_0001.bin                                   │
│  └─ ...                                              │
└────────────────────────────────────────────────────────┘
```

### Implementation Pseudocode

```cpp
class OpticalFlowEffect : public ImageEffect {
private:
    // Tier 1: GPU cache (fastest)
    struct GPUCacheEntry {
        wgpu::Texture resultTexture;
        wgpu::Buffer flowField;
        int lastAccessFrame;  // for LRU
    };
    std::map<int, GPUCacheEntry> _gpuCache;
    const size_t MAX_GPU_FRAMES = 30;

    // Tier 2: CPU compressed cache (fast)
    std::map<int, std::vector<uint8_t>> _cpuCache;
    const size_t MAX_CPU_FRAMES = 100;

    // Tier 3: Disk cache (persistent)
    std::filesystem::path _diskCachePath;

public:
    OfxStatus render(const RenderArguments& args) override {
        int frame = (int)args.time;

        // Try GPU cache first
        if (_gpuCache.count(frame)) {
            auto& entry = _gpuCache[frame];
            entry.lastAccessFrame = _currentPlayheadFrame;
            copyTextureToOutput(entry.resultTexture, dstImage);
            return kOfxStatOK;
        }

        // Try CPU cache second
        if (_cpuCache.count(frame)) {
            wgpu::Texture tex = decompressAndUploadToGPU(_cpuCache[frame]);
            storeInGPUCache(frame, tex);
            copyTextureToOutput(tex, dstImage);
            return kOfxStatOK;
        }

        // Try disk cache third
        std::string diskPath = _diskCachePath / ("frame_" + pad(frame, 4) + ".bin");
        if (std::filesystem::exists(diskPath)) {
            auto compressed = loadFromDisk(diskPath);
            wgpu::Texture tex = decompressAndUploadToGPU(compressed);
            storeInCPUCache(frame, compressed);
            storeInGPUCache(frame, tex);
            copyTextureToOutput(tex, dstImage);
            return kOfxStatOK;
        }

        // Cache miss - must render
        wgpu::Texture result = computeOpticalFlowFrame(frame);

        // Store in all cache tiers
        storeInGPUCache(frame, result);
        auto compressed = compressTexture(result);
        storeInCPUCache(frame, compressed);
        saveToDisk(frame, compressed);

        copyTextureToOutput(result, dstImage);
        return kOfxStatOK;
    }

private:
    void storeInGPUCache(int frame, wgpu::Texture tex) {
        // LRU eviction if needed
        if (_gpuCache.size() >= MAX_GPU_FRAMES) {
            evictOldestGPUFrame();
        }

        _gpuCache[frame] = {tex, nullptr, _currentPlayheadFrame};
    }

    void evictOldestGPUFrame() {
        int oldestFrame = -1;
        int oldestAccess = INT_MAX;

        for (auto& [frame, entry] : _gpuCache) {
            if (entry.lastAccessFrame < oldestAccess) {
                oldestAccess = entry.lastAccessFrame;
                oldestFrame = frame;
            }
        }

        if (oldestFrame >= 0) {
            _gpuCache.erase(oldestFrame);
        }
    }
};
```

---

## Part 9: Progressive Simulation Integration

### Recall: The Progressive Simulation Pattern

From your original design (see `WebGPUSimulationCache.md`):

1. User clicks "Simulate" button
2. Background thread runs simulation for all frames
3. Hidden `_progressParam` is updated each frame
4. Parameter change triggers host to invalidate → call `render()`
5. `render()` shows progress overlay while simulation runs
6. After simulation completes, `render()` uses cached results

### How This Works With WebGPU Cache

```cpp
void runSimulationLoop() {
    progressStart("Simulating Optical Flow...");

    wgpu::Buffer accumulatedFlow = nullptr;

    for (int frame = start; frame <= end; frame++) {
        // WebGPU compute shader: optical flow
        wgpu::Buffer currentFlow = computeOpticalFlow(frame);

        // WebGPU compute shader: fluid accumulation
        accumulatedFlow = accumulateFluid(accumulatedFlow, currentFlow);

        // WebGPU render: final frame
        wgpu::Texture result = renderFluidEffect(accumulatedFlow);

        // ⭐ Store in cache tiers
        storeInGPUCache(frame, result, currentFlow);
        auto compressed = compressTexture(result);
        storeInCPUCache(frame, compressed);
        saveToDiskAsync(frame, compressed);

        // ⭐ Trigger host redraw via progress parameter
        _progressParam->setValue(frame);

        double progress = (double)(frame - start + 1) / totalFrames;
        if (!progressUpdate(progress)) {
            // User cancelled
            break;
        }
    }

    progressEnd();
    _simulationRunning = false;
}
```

---

## Part 10: Key Takeaways for WebGPU Implementation

### 1. Host Manages Output Cache, Plugin Manages Internal State

- **Host cache**: Final RGBA pixel buffers (what user sees)
- **Plugin cache**: WebGPU simulation state (flow fields, accumulated data)

### 2. Use GetFramesNeeded to Declare Dependencies

```cpp
OfxStatus getFramesNeeded(OfxPropertySetHandle inArgs,
                          OfxPropertySetHandle outArgs) {
    double time;
    gPropHost->propGetDouble(inArgs, kOfxPropTime, 0, &time);

    // Optical flow needs previous frame
    double range[2] = {time - 1, time};
    gPropHost->propSetDoubleN(outArgs,
        "OfxImageClipPropFrameRange_Source", 2, range);

    return kOfxStatOK;
}
```

### 3. Prefer Sequential Rendering for Simulations

```cpp
// In describe()
gPropHost->propSetInt(effectProps,
    kOfxImageEffectInstancePropSequentialRender, 0, 2);
```

### 4. Use BeginSequenceRender/EndSequenceRender

Initialize simulation state in `beginSequenceRender()`, clean up in `endSequenceRender()`.

### 5. Hybrid Cache Strategy

- GPU cache (hot, 30 frames)
- CPU compressed cache (warm, 100 frames)
- Disk cache (cold, all frames)

### 6. SyncPrivateData for Persistence

Serialize disk cache path to parameters so project saves work.

### 7. Hidden Progress Parameter for Progressive Simulation

Update hidden param → host invalidates → `render()` called → show progress overlay.

---

## Summary

| Aspect | OFX Mechanism | WebGPU Implementation |
|--------|---------------|----------------------|
| **Frame cache** | Host-managed (automatic) | Plugin-managed (explicit) |
| **Temporal dependencies** | `GetFramesNeeded` action | Declare previous frame dependency |
| **Sequential order** | `SequentialRender` property | Required for simulation accumulation |
| **Sequence lifecycle** | `Begin/EndSequenceRender` | Initialize/cleanup WebGPU state |
| **State persistence** | `SyncPrivateData` action | Serialize cache to disk |
| **Cache storage** | Host RAM | Hybrid GPU/CPU/Disk |
| **Cache size** | Host decides | Plugin manages (LRU eviction) |
| **Progressive simulation** | Hidden progress parameter | Update param → trigger redraw |

---

## Next Steps

1. Implement basic OFX plugin skeleton with WebGPU initialization
2. Add `GetFramesNeeded` to declare temporal dependencies
3. Implement hybrid cache (GPU → CPU → Disk)
4. Add sequential rendering with `Begin/EndSequenceRender`
5. Implement progressive simulation with background thread
6. Add `SyncPrivateData` for project persistence
7. Test with `testSDL2` or `hostDemo`

---

**Document created**: 2025-10-19
**Purpose**: Understanding OFX caching for WebGPU Optical Flow Fluid plugin
**References**: OFX 1.4 specification, HostSupport library, previous conversation (`WebGPUSimulationCache.md`)
