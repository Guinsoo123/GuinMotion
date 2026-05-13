# GuinMotion 技术选型

## 设计目标

GuinMotion 是一款用于机器人运动控制算法开发与验证的跨平台桌面软件，首批目标平台是 macOS 和 Ubuntu。软件需要对 C++ 算法开发者友好，支持 Python 实验算子，支持点云、点、路点和轨迹可视化，并尽量做到低依赖、可离线安装、开箱即用。

技术选型围绕以下目标展开：

- 运动控制、几何计算、轨迹处理需要原生性能。
- 便于导入现有 C++ 算法、机器人模型和厂商 SDK。
- macOS 和 Ubuntu 的打包路径要清晰可控。
- 运行时依赖要少，避免要求用户手动安装复杂环境。
- 插件机制要可扩展，核心程序不应绑定所有算法库。

## 推荐技术栈

| 领域 | 推荐方案 | 原因 |
| --- | --- | --- |
| 主语言 | C++20 | 适合机器人、数值计算、二进制插件和长期 ABI 管理。 |
| GUI | Qt 6 Widgets | 成熟稳定、跨平台、CMake 集成好，打包复杂度低于 Web 外壳。 |
| 3D 渲染 | Qt OpenGL / Qt RHI 后端封装为 `Viewport` 接口 | 第一版简单可靠，后续可迁移到 Vulkan/Metal。 |
| 点云与几何 | 核心自有轻量数据模型，Open3D C++ 作为可选后端 | 避免核心数据被重型库绑定，同时保留点云算法能力。 |
| 机器人模型 | 先定义内部接口，后续通过适配器支持 URDF、Pinocchio、RBDL 或厂商 SDK | 不让核心依赖某个机器人生态。 |
| 构建系统 | CMake | 适合 C++、Qt、pybind11、插件和跨平台 CI。 |
| 依赖管理 | vcpkg manifest 模式 | 可锁版本、可复现、便于 CI 和内部镜像。 |
| C++ 算子插件 | 动态库 + 稳定 C ABI 入口 | 跨编译器兼容性更好，加载边界清晰。 |
| Python 算子 | 嵌入 CPython + pybind11 | 保持主程序原生，同时支持 Python 算法验证。 |
| 发布包 | macOS `.app/.dmg`，Ubuntu 优先 AppImage | 最符合“开箱即用”的目标。 |

## 主语言：C++20

C++20 应作为 GuinMotion 的主开发语言。它可以直接承载高性能运动规划、点云处理、二进制插件加载和机器人算法集成。

关键约束：

- 应用内部使用现代 C++，但插件 ABI 边界保持 C 兼容。
- 不在动态库边界暴露 STL 容器、异常或编译器相关 C++ 符号。
- 领域对象要便于序列化，能在 UI、可视化和算子运行时之间安全传递。
- 暂不依赖 C++23 特性，避免 macOS/Ubuntu 编译器和打包兼容性风险。

## 桌面 UI：Qt 6 Widgets

第一版推荐 Qt 6 Widgets。它没有 QML 那么适合做复杂动效，但对工程工具更稳定、可预测，打包也更容易。

不优先选择 Electron 的原因：

- 包体积和内存占用明显增加。
- C++ 算法集成通常需要 native module 或本地服务。
- 与低依赖、开箱即用的目标不一致。

不优先选择 Python UI 的原因：

- 二进制依赖较多时，Python 桌面应用打包更脆弱。
- C++ 插件加载和 ABI 管理会变得间接。
- 长时间几何计算容易与 Python 运行时限制冲突。

QML 可作为后续局部增强方案，但第一版架构不应依赖 QML。

## 可视化方案

GuinMotion 应拥有自己的轻量可视化数据模型，并通过接口暴露渲染能力：

- `PointCloudLayer`：点、颜色、法向、标签。
- `WaypointLayer`：命名机器人关节状态和笛卡尔标记。
- `TrajectoryLayer`：有序路点、时间、插值和样式。
- `RobotModelLayer`：连杆、关节、碰撞几何和当前状态。

第一版渲染器可使用 Qt OpenGL 集成。Open3D C++ 适合作为几何和点云处理后端，而不是整个应用的数据模型。

这种设计可以让 GuinMotion 不依赖 ROS/RViz。ROS 支持后续可以作为适配器加入，而不是核心依赖。

## 算子运行时

已选定的算子模型是进程内算子：

- C++ 算子通过动态库加载，macOS 为 `.dylib`，Ubuntu 为 `.so`。
- Python 算子通过嵌入 CPython 和 pybind11 桥接运行。
- 两类算子共用统一的 metadata、输入、输出和生命周期模型。

这种方案对早期版本的性能和开发效率最好。主要风险是错误算子可能导致宿主进程崩溃。因此运行时需要定义严格边界、版本校验、输入验证，并保留未来支持进程外沙箱算子的路径。

## 依赖策略

GuinMotion 应把依赖分为三层：

1. 启动核心应用必须依赖的基础库。
2. 官方模块随包携带的功能依赖。
3. 仅特定插件使用的可选依赖。

核心依赖应尽量少：

- Qt 6。
- C++ 标准库和平台运行时。
- 小型 JSON/YAML 解析库，用于项目文件和算子 metadata。
- Python 算子开启时，才需要嵌入式 Python runtime。

第一版不应强制依赖：

- ROS 2。
- 全局 PCL。
- CUDA。
- 厂商机器人 SDK。
- 数据库引擎。
- Web runtime 外壳。

## 构建与发布

CMake 顶层建议拆出这些目标：

- `guinmotion_app`：桌面应用。
- `guinmotion_core`：领域模型和核心服务。
- `guinmotion_visualization`：可视化模型和渲染集成。
- `guinmotion_operator_runtime`：插件管理和运行时。
- `guinmotion_sdk`：外部算子开发所需头文件和帮助库。

vcpkg manifest 模式负责锁定第三方依赖。后续 CI 可构建：

- macOS Apple Silicon 或 universal 包。
- Ubuntu x86_64 AppImage。
- 可选开发者 SDK 包，包含头文件、CMake 配置和示例算子。

## 最终建议

GuinMotion 第一版建议采用：

- C++20。
- Qt 6 Widgets。
- CMake + vcpkg manifest 模式。
- 内部数据模型覆盖点、点云、路点、轨迹、机器人状态和算子消息。
- C++ 动态库算子使用稳定 C ABI。
- Python 算子使用嵌入 CPython + pybind11。
- macOS 使用 `.app/.dmg`，Ubuntu 使用 AppImage。

这套技术栈最符合 GuinMotion 的定位：原生、高性能、跨平台、可扩展的机器人运动控制算法开发与可视化验证工具。
