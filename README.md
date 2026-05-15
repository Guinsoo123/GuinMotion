# GuinMotion

GuinMotion 是一个面向机器人运动控制算法开发与验证的跨平台原生工程软件骨架。当前仓库按 `design/` 设计文档落地了第一阶段代码结构，重点覆盖：

- C++20 + CMake 工程骨架。
- 核心领域模型：点云、路点、轨迹、机器人模型、项目场景。
- 算子 SDK 与 C ABI 插件入口草案。
- 示例 C++ 算子插件。
- 可运行 Qt 6 Widgets 桌面界面（默认构建启用），可选无界面 headless 预设。
- macOS / Ubuntu 的安装依赖、编译运行、打包脚本。

## 目录结构

```text
GuinMotion/
  3rdparty/              # 第三方库说明、后续 vendored 依赖入口
  design/                # 中文设计文档
  resources/             # 应用资源
  script/                # 一键安装、编译运行、打包脚本
  src/
    app/                 # 应用入口
    core/                # 领域模型和项目服务
    operator_runtime/    # 算子注册、插件 ABI、运行时骨架
    sdk/                 # 第三方算子开发 SDK 目标
    plugins/examples/    # 示例算子插件
  tests/                 # 后续测试目录
```

## 快速开始

下面流程目标是：依赖装完后，直接运行一条脚本即可完成配置、编译并启动程序。

### 1. 安装依赖

macOS 和 Ubuntu 都优先使用一键脚本：

```bash
./script/install_deps.sh
```

脚本会自动识别当前系统：

- macOS：使用 Xcode Command Line Tools + Homebrew 安装依赖。
- Ubuntu/Debian：使用 `apt-get` 安装依赖。
- 其它 Linux 发行版：会明确提示暂不支持自动安装，需要手动安装 C++ 编译器、CMake、Ninja 和 `pkg-config`。

该脚本会安装或检查默认构建所需依赖：

- C++ 编译器：macOS 使用 Xcode Command Line Tools，Ubuntu 使用 `build-essential`。
- CMake。
- Ninja。
- `pkg-config`。
- `git`、`curl`、`zip`、`unzip`、`tar` 等 vcpkg 需要的基础工具。
- **Qt 6 与 OpenGL**：默认安装 `qt6-base-dev`、`qt6-base-dev-tools`、`libgl1-mesa-dev`（Ubuntu）；macOS 通过 Homebrew 安装 `qtbase`。
- **MuJoCo / Assimp / tinyxml2**：脚本会在项目本地 `.deps/vcpkg` 克隆/复用 vcpkg，并按 `vcpkg.json` 安装 MuJoCo 仿真、Assimp 网格、tinyxml2 XML 相关依赖。`build_and_run.sh` 会自动使用这个 vcpkg toolchain。

macOS 有两个可能需要手动确认的系统步骤：

1. 如果还没有 Xcode Command Line Tools，脚本会触发系统安装提示。安装完成后需要重新运行：

   ```bash
   ./script/install_deps.sh
   ```

2. 如果还没有 Homebrew，需要先按 [https://brew.sh](https://brew.sh) 的官方命令安装 Homebrew，然后重新运行安装脚本。

Ubuntu 需要当前用户具备 `sudo` 权限，因为脚本会调用 `apt-get` 安装系统包。

### 2. 编译并启动桌面应用

默认 CMake 预设已启用 **Qt 6 Widgets**，`build_and_run.sh` 在编译成功后会在前台 **启动图形窗口**（关闭窗口即结束进程）。

```bash
./script/build_and_run.sh
```

`build_and_run.sh` 会识别 macOS / Ubuntu，并在构建前检查 `c++`、`cmake`、`ninja`。在 **macOS** 上会自动把 Homebrew Qt 加入 `CMAKE_PREFIX_PATH`，同时自动传入 `.deps/vcpkg/scripts/buildsystems/vcpkg.cmake`，确保 MuJoCo/Assimp/tinyxml2 被 CMake 找到。

构建成功后脚本会先运行 CTest 和 `guinmotion_one_stop_validation`，确认 MuJoCo 仿真/轨迹评价链路通过，再启动 GUI。

可选环境变量：

- `GUINMOTION_PRESET`：例如 `default`、`release`、`headless`（无 Qt，仅命令行输出）。
- 无界面机器上安装依赖时可设 `GUINMOTION_SKIP_QT=1`，再用 `GUINMOTION_PRESET=headless ./script/build_and_run.sh` 编译运行。

无界面（`headless`）运行成功时终端会看到类似输出：

```text
GuinMotion GuinMotion Demo Project
机器人：1
点云：1
轨迹：2
路点：4
已注册算子：3
```

启用 Qt 时上述统计信息显示在窗口内，而不是终端。

### 2.1 一站式验证示例

构建后可单独运行端到端示例：

```bash
./build/default/bin/guinmotion_one_stop_validation
```

示例会导入 `examples/one_stop_validation/data` 中的 URDF、目标点 XML 和点云，运行目标点演示算子、执行仿真 trace，并输出 `evaluation: PASS`。

### 3. 打包

生成 release 构建并调用 CPack：

```bash
./script/package.sh
```

`package.sh` 使用带 Qt 的 `release` 预设；在 **macOS** 上同样会自动设置 Qt 的 `CMAKE_PREFIX_PATH`。

macOS 默认生成 `.dmg` 和 `.tar.gz`/`.tgz` 类型产物；Ubuntu 默认生成 `.tar.gz`/`.tgz` 类型产物。后续接入 AppImage 工具链后，可在 `script/package.sh` 中扩展 AppImage 输出。

### 4. 可选：无 Qt / 仅命令行

若不需要图形界面（例如 CI 或精简环境）：

1. 安装依赖时跳过 Qt：`GUINMOTION_SKIP_QT=1 ./script/install_deps.sh`
2. 使用无 Qt 预设构建运行：`GUINMOTION_PRESET=headless ./script/build_and_run.sh`

### 5. 手动指定 Qt 路径（macOS）

若 CMake 仍找不到 Qt，可在配置前设置：

```bash
export CMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --preset default
```

或 Qt 6 单独前缀：

```bash
export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
```

### 常见问题

- `cmake not found`：先运行 `./script/install_deps.sh`。
- `No CMAKE_CXX_COMPILER could be found`：macOS 运行 `xcode-select --install`，Ubuntu 运行 `sudo apt-get install build-essential`。
- `Ninja not found`：重新运行 `./script/install_deps.sh`，或手动安装 `ninja` / `ninja-build`。
- macOS 第一次安装 Xcode Command Line Tools 后，需要重新打开终端或重新运行安装脚本。
- **找不到 Qt6**：先执行 `./script/install_deps.sh`；仍失败时用上一节的 `CMAKE_PREFIX_PATH`。

## 设计文档

入口见 [`design/README.md`](design/README.md)。
