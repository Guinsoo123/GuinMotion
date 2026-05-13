# 跨平台构建与打包

## 目标

GuinMotion 应能在 macOS 和 Ubuntu 上以尽量少的配置运行。开发者可以从源码构建，普通用户则应能安装发布包后直接开始验证机器人运动控制算法，而不需要手动安装大量依赖。

主要目标：

- 使用同一套 CMake 构建系统。
- 第三方依赖版本可复现。
- 产出原生应用发布包。
- 随包携带官方插件和示例。
- 插件目录和 Python runtime 布局清晰。
- 核心应用不要求安装 ROS。

## 目标平台

首批平台：

- macOS 14 或更新版本，优先 Apple Silicon。
- Ubuntu 22.04 LTS 和 24.04 LTS，优先 x86_64。

未来平台：

- macOS universal build。
- Ubuntu ARM64。
- Windows，待插件 ABI 和可视化层稳定后再考虑。

## 仓库布局建议

后续工程目录建议如下：

```text
GuinMotion/
  CMakeLists.txt
  CMakePresets.json
  vcpkg.json
  app/
  core/
  visualization/
  operator_runtime/
  sdk/
  plugins/
    builtin/
    examples/
  python/
    guinmotion_sdk/
  resources/
  tests/
  design/
```

当前任务只创建设计文档。上面的结构是后续实现时的目标工程布局。

## CMake 目标

推荐目标边界：

- `guinmotion_core`：项目模型、领域数据、文件适配器、命令系统。
- `guinmotion_visualization`：场景图、可视化图层、渲染集成。
- `guinmotion_operator_runtime`：插件发现、C++ 加载器、嵌入式 Python 宿主。
- `guinmotion_sdk`：第三方算子开发所需头文件和帮助库。
- `guinmotion_app`：Qt 桌面可执行程序。
- `guinmotion_builtin_plugins`：随应用发布的官方算子。
- `guinmotion_tests`：单元测试和集成测试。

应用目标依赖模块库。插件目标只依赖 SDK 和插件自身需要的依赖。

## 依赖管理

使用 vcpkg manifest 模式管理 C++ 第三方依赖：

- 依赖版本由 baseline 或 `vcpkg-configuration.json` 锁定。
- CI 和本地构建使用同一份依赖列表。
- CI 可以缓存依赖构建结果。
- 未来可以接入离线依赖包或内部镜像。

推荐依赖策略：

- 核心应用只依赖稳定、常见的库。
- 重型依赖保持可选或由插件持有。
- 厂商 SDK 不作为全局依赖。
- Python runtime 由构建选项控制。

依赖分组示例：

- 必需依赖：Qt 6、小型 JSON 库、测试框架。
- 官方可选模块：Open3D、pybind11、Python runtime。
- 插件专属依赖：机器人厂商 SDK、特殊优化库。

## CMake Presets

使用 CMake presets 保证构建一致：

- `macos-debug`。
- `macos-release`。
- `ubuntu-debug`。
- `ubuntu-release`。
- `ci-macos`。
- `ci-ubuntu`。

Presets 应定义编译器、构建类型、vcpkg toolchain、Qt 路径、Python 算子开关和打包选项。

## 发布包布局

### macOS

推荐 `.app` 布局：

```text
GuinMotion.app/
  Contents/
    MacOS/
      GuinMotion
    Frameworks/
      Qt frameworks
      bundled libraries
    Resources/
      app resources
      builtin plugins
      examples
      python runtime optional
    PlugIns/
      platforms/
      imageformats/
      guinmotion/
```

开发阶段使用 `.app`，面向用户发布使用 `.dmg`。外部分发前应规划代码签名和 notarization。

### Ubuntu

推荐 AppImage 布局：

```text
GuinMotion.AppDir/
  AppRun
  usr/
    bin/GuinMotion
    lib/
    share/guinmotion/
      resources/
      plugins/
      examples/
      python/
```

AppImage 最适合开箱即用；`.deb` 可作为受管工作站或企业环境补充。

## 插件目录布局

GuinMotion 应按确定顺序搜索插件：

1. 应用内置官方插件。
2. 用户插件目录。
3. 用户确认信任后的项目本地插件目录。

macOS：

```text
GuinMotion.app/Contents/Resources/plugins/
~/Library/Application Support/GuinMotion/plugins/
<project>/.guinmotion/plugins/
```

Ubuntu：

```text
<AppDir>/usr/share/guinmotion/plugins/
~/.local/share/GuinMotion/plugins/
<project>/.guinmotion/plugins/
```

C++ 插件目录建议：

```text
plugin-name/
  plugin.json
  libplugin_name.dylib or libplugin_name.so
  resources/
  examples/
```

Python 插件目录建议：

```text
plugin-name/
  plugin.json
  main.py
  requirements.lock optional
  resources/
```

## Python Runtime 打包

Python 支持应是可选但一等的能力。

推荐策略：

- 开启 Python 算子时，release 包内置固定 Python runtime。
- 开发构建允许指向本地 Python 安装。
- 官方 Python 算子依赖要 vendored 或锁版本。
- 项目本地 Python 插件不得未经用户确认自动安装包。

这样可以减少“在我机器上可用”的问题。

## CI 策略

初始 CI 任务：

- CMake configure。
- Debug 构建。
- Release 构建。
- 单元测试。
- 发布包烟测。
- 加载示例 C++ 插件。
- 若启用 Python runtime，则加载示例 Python 算子。

```mermaid
flowchart LR
  Commit["提交"] --> Configure["CMake 配置"]
  Configure --> Build["构建"]
  Build --> UnitTests["单元测试"]
  UnitTests --> PluginTests["插件加载测试"]
  PluginTests --> Package["创建发布包"]
  Package --> SmokeTest["启动烟测"]
```

## 发布产物

每个 release 建议包含：

- macOS `.dmg`。
- Ubuntu AppImage。
- 算子开发者 SDK 包。
- 示例插件包。
- 校验和。
- Release notes。

SDK 包应包含公共头文件、CMake package config、插件 manifest schema、最小 C++ 算子示例和最小 Python 算子示例。

## 运行时诊断

GuinMotion 启动时应能展示：

- 应用版本。
- 构建类型。
- 平台。
- Qt 版本。
- SDK ABI 版本。
- 插件目录。
- 已加载插件。
- 被拒绝插件及原因。
- Python runtime 状态。

这会显著提升部署和现场排查效率。

## 第一阶段实现路径

推荐构建里程碑：

1. CMake 骨架，包含 `core`、`app` 和 `tests`。
2. Qt 主窗口和空 3D 视口占位。
3. 领域模型单元测试。
4. C++ 插件 SDK 示例。
5. 插件发现和 metadata UI。
6. Python runtime 作为构建选项接入。
7. macOS `.app` 和 Ubuntu AppImage 烟测包。

这个顺序可以尽早得到可运行应用壳，同时在大量算法开发前降低插件和打包风险。
