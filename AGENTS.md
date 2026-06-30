# Language Manager - Agent Instructions

C++17 plugin-based G2p (grapheme-to-phoneme) framework. Core library in `core/`, plugins in `plugins/`, resources in `res/`.

## Build Commands

Build directory **must** be `build/`. Other dirs (`cmake-build-debug`, `cmake-build-release`) are for IDE use only.

```bash
# Configure (requires vcpkg + qmsetup + Qt in CMAKE_PREFIX_PATH)
cmake -B build ^
  -DCMAKE_TOOLCHAIN_FILE=D:\projects\ds-editor-lite\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DCMAKE_PREFIX_PATH=D:\Programs\qt5\6.10.2\msvc2022_64 ^
  -DCMAKE_INSTALL_PREFIX=install ^
  -DLANGMGR_BUILD_PLUGINS=ON ^
  -DLANGMGR_BUILD_TESTS=ON

# Build
cmake --build build --config Debug

# Test
ctest --test-dir build -C Debug
```

Optional GPU acceleration flags: `-DLANGPLUGINS_ENABLE_DIRECTML=ON`, `-DLANGPLUGINS_ENABLE_CUDA=ON`.

A full clean-build script exists at `test_build.bat` (hardcoded local paths).

**After every code change, build in `build/` and verify compilation passes.**

## Key Conventions

- **异常处理：** 应用层逻辑使用 `Expected<T>`，禁止抛出异常。try-catch 仅用于第三方库边界（JSON、ONNX Runtime、std::regex 等），将外部异常转为 `Error`。每个 catch 必须记录日志或返回错误，禁止静默吞掉。
- **Config access:** `auto cfg = LangCore::config(m_spec);` — use `getString("key")` (required) or `getString("key", "default")` (optional).
- **Logging:** `#include <LangCore/Support/Logging.h>` — declare a `LogCategory`, then call `Log.langCoreInfo(...)`, `Log.langCoreWarning(...)`, `Log.langCoreDebug(...)`, etc. Format uses `%1`, `%2` placeholders (Qt-style). `langCoreInfoF` variants use printf-style formatting.
- **Plugin export:** Use `LANGCORE_DEFINE_TASK_PLUGIN(PluginClass, TaskClass, "category.key", ApiLevel)` for task plugins, `LANGCORE_DEFINE_DRIVER_PLUGIN(...)` for drivers. These internally call `LANGCORE_EXPORT_PLUGIN`.
- **Naming:** classes `[Name]Plugin` / `[Name]Task`, namespace `LangPlugins::[Name]`, key `category.plugin-name`
- **Level** (integer) is the sole API compatibility check: `M-1 <= P <= M`. Version (semver) is used for dependency range filtering but does not affect API compatibility; first digit should match Level.
- **Code format:** `.clang-format` (LLVM-based, 4-space indent, 120 col limit, `NamespaceIndentation: All`, `BraceWrapping.AfterNamespace: true`)

## Architecture

- `core/include/LangCore/` — public headers (Base, Core, Module, Package, Support, Task)
- `core/lib/` — implementation
- `plugins/` — Dicts, Drivers, G2ps, Utils (each is a dynamically loaded plugin)
- `res/G2pPackages/` — package resources (model files, dicts)
- `tests/catch2/` — Catch2 L1/L2 unit & component tests (single executable `LangMgrTests`, no plugins or runtime needed)
- `tests/common/` — Catch2 v2.13.10 amalgamated header (catch.hpp)
- `tests/tst_langCore/` — integration tests (requires runtime plugins and packages; includes test-local Splitter/Tagger implementation)
- `docs/` — design docs (PRD, architecture, plugin dev guide, ChainG2p design, etc.)

### Plugin categories
- **CorePlugin** (G2p, Dict): uses `TaskInput`/`TaskResult`, requires Level check
- **UtilityPlugin** (Driver, helpers): independent, no Level check
- 3 module categories registered: `driver`, `g2p`, `dict`

> **Note:** Splitter and Tagger have been moved to frontend. Test-local implementations exist in `tests/tst_langCore/` with simplified JSON configs in `tests/tst_langCore/configs/`.

## Dependencies

- qmsetup (build), vcpkg, ONNX Runtime, cpp-pinyin, cpp-kana
- Qt 6 (for test infrastructure)

## Design Docs

Key references in `docs/`: `PRD-v2.0.md`, `Plugin-Development-Guide.md`, `ChainG2p-Design-Document.md`
