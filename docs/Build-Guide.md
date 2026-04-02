# Language Manager 构建指南

## 文档说明

本文档描述 Language Manager 的构建配置、依赖管理和构建步骤。

**版本**：1.0
**日期**：2026-04-02

---

## 目录

1. [构建环境](#1-构建环境)
2. [构建目录说明](#2-构建目录说明)
3. [依赖管理](#3-依赖管理)
4. [构建配置](#4-构建配置)
5. [构建步骤](#5-构建步骤)
6. [构建输出](#6-构建输出)
7. [IDE 配置](#7-ide-配置)

---

## 1. 构建环境

### 1.1 操作系统

- **Windows**: Windows 10/11 (MSVC 2019/2022)
- **Linux**: Ubuntu 20.04+ (GCC 9+)
- **macOS**: macOS 11+ (Clang 11+)

### 1.2 必需工具

- **CMake**: 3.19 或更高版本
- **vcpkg**: 依赖管理工具（用于安装 stdcorelib 和 nlohmann_json）
- **Qt**: 6.10.2（通过 qmsetup 集成）
- **qmsetup**: Qt 构建工具和部署系统

### 1.3 C++ 标准

- **C++17** 或更高版本

### 1.4 编译器要求

- **Windows**: MSVC 2019/2022
- **Linux**: GCC 9+
- **macOS**: Clang 11+

---

## 2. 构建目录说明

项目使用多个构建目录，各有不同用途：

| 目录 | 用途 | 说明 |
|------|------|------|
| `build/` | AI 自动编译测试 | 专用于 AI 修改代码时的自动编译测试和修复编译错误，不影响其他构建目录 |
| `cmake-build-debug/` | CLion Debug 构建 | CLion IDE 的 Debug 模式构建目录 |
| `cmake-build-release/` | CLion Release 构建 | CLion IDE 的 Release 模式构建目录 |
| `install/` | 安装目录 | CMake 安装目标输出目录 |

**注意**：`build/` 目录专用于 AI 修改代码时的自动编译测试和修复编译错误，不会影响其他构建目录（如 `cmake-build-debug/` 和 `cmake-build-release/`）。

---

## 3. 依赖管理

### 3.1 核心依赖（通过 vcpkg 安装）

**必需依赖**：
- **stdcorelib**: 提供智能指针、文件系统、线程等基础设施
- **nlohmann_json**: JSON 处理库

**依赖安装命令**：

```bash
# 安装 stdcorelib
vcpkg install stdcorelib

# 安装 nlohmann-json
vcpkg install nlohmann-json
```

### 3.2 运行时依赖

**必需依赖**：
- **Qt 6.10.2**: 通过 qmsetup 集成
- **ONNX Runtime**: AI 模型推理框架
- **cpp-pinyin**: 普通话拼音转换
- **cpp-kana**: 日语假名转换

**注意**：Qt 和 ONNX Runtime 通过 CMake 的 `CMAKE_PREFIX_PATH` 指定路径。

### 3.3 依赖路径配置

**vcpkg 工具链文件**：
- 路径：`path/to/vcpkg/scripts/buildsystems/vcpkg.cmake`

**Qt 前缀路径**：
- 路径：`path/to/qt/6.10.2/msvc2022_64`

**qmsetup 路径**：
- 通过 `CMAKE_PREFIX_PATH` 指定

---

## 4. 构建配置

### 4.1 CMake 配置参数

**必需参数**：

| 参数 | 说明 | 示例 |
|------|------|------|
| `-DCMAKE_TOOLCHAIN_FILE` | vcpkg 工具链文件 | `path/to/vcpkg/scripts/buildsystems/vcpkg.cmake` |
| `-DCMAKE_PREFIX_PATH` | Qt 和 qmsetup 前缀路径 | `path/to/qmsetup;path/to/qt/6.10.2/msvc2022_64` |
| `-DCMAKE_INSTALL_PREFIX` | 安装目录 | `install` |

**可选参数**：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `-DLANGMGR_BUILD_PLUGINS` | `OFF` | 构建插件 |
| `-DLANGMGR_BUILD_TESTS` | `OFF` | 构建测试 |
| `-DLANGCORE_INSTALL` | `ON` | 安装核心库 |
| `-DLANGCORE_INSTALL_DEPLOY` | `OFF` | 安装部署依赖 |
| `-DLANGPLUGINS_ENABLE_DIRECTML` | `ON` | 启用 DirectML 支持 |
| `-DLANGPLUGINS_ENABLE_CUDA` | `ON` | 启用 CUDA 支持 |
| `-DLANGPLUGINS_ENABLE_STATIC_PLUGINS` | `OFF` | 启用静态插件链接（待实现） |

### 4.2 CMake 配置命令

**Windows (MSVC)**：

```bash
# 使用 CMake 生成器
cmake -S . -B build ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE="path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
  -DCMAKE_PREFIX_PATH="path/to/qmsetup;path/to/qt/6.10.2/msvc2022_64" ^
  -DCMAKE_INSTALL_PREFIX="install" ^
  -DLANGMGR_BUILD_PLUGINS=ON ^
  -DLANGMGR_BUILD_TESTS=ON ^
  -DLANGPLUGINS_ENABLE_DIRECTML=ON

# 使用 NMake 生成器
cmd /c '"path/to/vs2026/VC/Auxiliary/Build/vcvarsall.bat" x64 && cmake -S . -B build -G "NMake Makefiles" -DCMAKE_TOOLCHAIN_FILE="path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_PREFIX_PATH="path/to/qmsetup;path/to/qt/6.10.2/msvc2022_64" -DCMAKE_INSTALL_PREFIX="install" -DLANGMGR_BUILD_PLUGINS=ON -DLANGMGR_BUILD_TESTS=ON'
```

**Linux (GCC)**：

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=path/to/qmsetup;path/to/qt/6.10.2/gcc_64 \
  -DCMAKE_INSTALL_PREFIX=install \
  -DLANGMGR_BUILD_PLUGINS=ON \
  -DLANGMGR_BUILD_TESTS=ON \
  -DLANGPLUGINS_ENABLE_CUDA=ON
```

**macOS (Clang)**：

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=path/to/qmsetup;path/to/qt/6.10.2/clang_64 \
  -DCMAKE_INSTALL_PREFIX=install \
  -DLANGMGR_BUILD_PLUGINS=ON \
  -DLANGMGR_BUILD_TESTS=ON
```

---

## 5. 构建步骤

### 5.1 完整构建流程

**1. 安装依赖**：

```bash
# 使用 vcpkg 安装必需的依赖
vcpkg install stdcorelib
vcpkg install nlohmann-json
```

**2. 配置项目**：

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=path/to/qmsetup;path/to/qt/6.10.2/msvc2022_64 \
  -DCMAKE_INSTALL_PREFIX=install \
  -DLANGMGR_BUILD_PLUGINS=ON \
  -DLANGMGR_BUILD_TESTS=ON
```

**3. 构建**：

```bash
# Windows
cmake --build build --config Release

# Linux/macOS
cmake --build build -j$(nproc)
```

**4. 安装**：

```bash
# Windows
cmake --install build --config Release

# Linux/macOS
cmake --install build
```

### 5.2 快速构建（仅构建）

```bash
# 配置并构建（不安装）
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=path/to/qmsetup;path/to/qt/6.10.2/msvc2022_64 \
  -DLANGMGR_BUILD_PLUGINS=ON \
  -DLANGMGR_BUILD_TESTS=ON && \
cmake --build build --config Release
```

### 5.3 清理构建

```bash
# 删除构建目录
rm -rf build

# 或使用 CMake 清理
cmake --build build --target clean
```

---

## 6. 构建输出

### 6.1 输出目录

**构建输出目录**：
- Windows: `build/out-amd64-Release/`
- Linux/macOS: `build/out-x86_64-Release/`

### 6.2 核心库

**动态链接库**：
- Windows: `bin/LangCore.dll`
- Linux: `lib/libLangCore.so`
- macOS: `lib/libLangCore.dylib`

**导入库**：
- Windows: `lib/LangCore.lib`
- Linux: `lib/libLangCore.a`
- macOS: `lib/libLangCore.a`

### 6.3 测试程序

**测试可执行文件**：
- `bin/tst_langCore.exe` (Windows)
- `bin/tst_langCore` (Linux/macOS)

### 6.4 插件

**插件列表**：

1. **Drivers/OnnxDriver** - ONNX Runtime 驱动插件
   - 支持 CUDA 和 DirectML
   - 用于 AI 模型推理

2. **Splitters/RegexTagger** - 正则表达式文本分割器
   - 基于正则表达式的文本分割
   - 支持自定义分割规则

3. **Taggers/TemplateTagger** - 模板语言标记器
   - 基于模板的语言标记
   - 支持多语言识别

4. **G2ps** - G2p 处理器插件组：
   - **CantoneseG2p** - 粤语 G2p
   - **LstmG2p** - LSTM G2p
   - **MandarinG2p** - 普通话 G2p
   - **TemplateG2p** - 模板 G2p

### 6.5 工具库

**工具库列表**：
- `lib/InferUtil.dll` (Windows) / `libInferUtil.so` (Linux) - 推理工具库
- `lib/OnnxUtil.dll` (Windows) / `libOnnxUtil.so` (Linux) - ONNX 工具库

---

## 7. IDE 配置

### 7.1 CLion 配置

**CMake 选项**（CLion 设置）：

```cmake
-DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
-DCMAKE_PREFIX_PATH=path/to/qmsetup;path/to/qt/6.10.2/msvc2022_64
-DCMAKE_INSTALL_PREFIX=install
-DLANGMGR_BUILD_PLUGINS=ON
-DLANGMGR_BUILD_TESTS=ON
-DLANGPLUGINS_ENABLE_DIRECTML=ON
```

**工具链设置**：
- Windows: `D:\Programs\vs2026` (Visual Studio 2026)
- Linux: 使用系统默认 GCC
- macOS: 使用系统默认 Clang

**构建类型**：
- Debug: `cmake-build-debug/`
- Release: `cmake-build-release/`

### 7.2 Visual Studio 配置

**CMake Settings.json**：

```json
{
  "configurations": [
    {
      "name": "x64-Debug",
      "generator": "Visual Studio 17 2022",
      "configurationType": "Debug",
      "buildRoot": "${projectDir}\\build\\${name}",
      "installRoot": "${projectDir}\\install\\${name}",
      "cmakeCommandArgs": "-DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=path/to/qmsetup;path/to/qt/6.10.2/msvc2022_64",
      "buildCommandArgs": "",
      "ctestCommandArgs": "",
      "inheritEnvironments": [ "msvc_x64_x64" ],
      "variables": [
        {
          "name": "LANGMGR_BUILD_PLUGINS",
          "value": "ON",
          "type": "BOOL"
        },
        {
          "name": "LANGMGR_BUILD_TESTS",
          "value": "ON",
          "type": "BOOL"
        },
        {
          "name": "LANGPLUGINS_ENABLE_DIRECTML",
          "value": "ON",
          "type": "BOOL"
        }
      ]
    },
    {
      "name": "x64-Release",
      "generator": "Visual Studio 17 2022",
      "configurationType": "Release",
      "buildRoot": "${projectDir}\\build\\${name}",
      "installRoot": "${projectDir}\\install\\${name}",
      "cmakeCommandArgs": "-DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=path/to/qmsetup;path/to/qt/6.10.2/msvc2022_64",
      "buildCommandArgs": "",
      "ctestCommandArgs": "",
      "inheritEnvironments": [ "msvc_x64_x64" ],
      "variables": [
        {
          "name": "LANGMGR_BUILD_PLUGINS",
          "value": "ON",
          "type": "BOOL"
        },
        {
          "name": "LANGMGR_BUILD_TESTS",
          "value": "ON",
          "type": "BOOL"
        },
        {
          "name": "LANGPLUGINS_ENABLE_DIRECTML",
          "value": "ON",
          "type": "BOOL"
        }
      ]
    }
  ]
}
```

### 7.3 VS Code 配置

**settings.json**：

```json
{
  "cmake.configureArgs": [
    "-DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake",
    "-DCMAKE_PREFIX_PATH=path/to/qmsetup;path/to/qt/6.10.2/msvc2022_64",
    "-DCMAKE_INSTALL_PREFIX=install",
    "-DLANGMGR_BUILD_PLUGINS=ON",
    "-DLANGMGR_BUILD_TESTS=ON",
    "-DLANGPLUGINS_ENABLE_DIRECTML=ON"
  ],
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "cmake.defaultVariants": {
    "buildType": {
      "default": "release",
      "choices": {
        "debug": {
          "short": "Debug",
          "long": "Debug",
          "buildType": "Debug"
        },
        "release": {
          "short": "Release",
          "long": "Release",
          "buildType": "Release"
        }
      }
    }
  }
}
```

---

## 8. 常见问题

### Q1: 构建时出现"找不到 stdcorelib"错误怎么办？

这是因为没有通过 vcpkg 安装 stdcorelib。请执行：
```bash
vcpkg install stdcorelib
```

### Q2: 构建时出现"找不到 qmsetup"错误怎么办？

这是因为没有正确设置 CMAKE_PREFIX_PATH。请确保：
1. qmsetup 已安装
2. 在 CMake 配置时添加：`-DCMAKE_PREFIX_PATH=path/to/qmsetup;path/to/qt/6.10.2/msvc2022_64`

### Q3: 如何只构建核心库，不构建插件？

在 CMake 配置时设置：
```bash
-DLANGMGR_BUILD_PLUGINS=OFF
```

### Q4: 如何启用 CUDA 支持？

在 CMake 配置时设置：
```bash
-DLANGPLUGINS_ENABLE_CUDA=ON
```

### Q5: 如何启用 DirectML 支持？

在 CMake 配置时设置：
```bash
-DLANGPLUGINS_ENABLE_DIRECTML=ON
```

### Q6: 构建失败，提示找不到 Qt 头文件？

请确保：
1. Qt 6.10.2 已正确安装
2. CMAKE_PREFIX_PATH 包含 Qt 路径
3. 使用正确的 Qt 编译器版本（MSVC 2022 对应 msvc2022_64）

---

**文档结束**