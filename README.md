# MEC设备软件系统 (v2.0)

这是一个基于Linux C/C++开发的工业级MEC（移动边缘计算）设备软件系统，专为车路协同（V2X）场景设计，用于处理多传感器数据融合、目标跟踪及标准化消息分发。

## 🌟 2.0 版本重大更新

本版本对核心架构进行了深度重构，引入了以下工业级特性：

1. **异步消息队列架构**：
   - 实现了基于生产者-消费者模型的线程安全循环队列。
   - 传感器数据处理与融合逻辑解耦，全链路延迟降低至微秒级。
2. **高阶卡尔曼滤波 (CA模型)**：
   - 升级为 6 维状态空间的恒定加速度（Constant Acceleration）运动模型。
   - 引入马氏距离（Mahalanobis Distance）进行目标关联，显著提升拥堵场景下的匹配鲁棒性。
3. **V2X 国标协议支持 (GB/T 31024)**：
   - 内置 RSM（路侧安全消息）编码器，支持将融合结果序列化为标准二进制协议包。
4. **集成化模拟器 (Simulator)**：
   - 支持通过本地场景脚本（Scenario Scripts）回放传感器数据，实现脱离硬件的算法回归测试。
5. **工程自动化与鲁棒性**：
   - 引入 DFA 雷达协议解析状态机，增强串口抗干扰能力。
   - 支持 `SIGHUP` 信号触发的配置热重载（Hot-Reload）。
   - 内置系统心跳监控与内存安全管理。

## 系统架构

系统包含以下核心组件：

- **感知层**：视频处理（OpenCV 4.x）、毫米波雷达解析（DFA状态机）。
- **交换层**：异步消息队列（MEC Queue）。
- **算法层**：多目标关联（马氏距离）、运动预测（CA Kalman Filter）。
- **应用层**：V2X 消息封装（RSM）、终端日志输出、场景模拟器。

## 项目结构

```
mec-system/
├── include/               # 接口定义 (.h)
│   ├── mec_queue.h        # 异步队列接口
│   ├── mec_fusion.h       # 融合算法定义
│   ├── mec_v2x.h          # 国标协议定义
│   └── mec_simulator.h    # 模拟器接口
├── src/
│   ├── common/            # 基础设施：队列、模拟器、协议编解码、内存、日志
│   ├── video/             # 视频预处理（C++ OpenCV）
│   ├── radar/             # 雷达解析（DFA 状态机）
│   └── fusion/            # 核心融合算法实现
└── config/
    ├── mec.conf           # 系统主配置文件
    └── scenario_test.txt  # 仿真场景描述文件
```

## 构建和运行

### 依赖环境
- CMake 3.10+
- GCC/G++ (支持 C11/C++11)
- OpenCV 4.x
- Pthread

### 快速启动
1. **构建项目**：
```bash
./build.sh
```

2. **运行模拟测试模式**：
```bash
./build/mec_system --sim
```

3. **运行真实传感器模式**：
```bash
./build/mec_system -c config/mec.conf
```

## 配置说明

支持通过 `config/mec.conf` 调整以下关键参数：
- `fusion.association_threshold`：目标关联距离阈值。
- `fusion.max_track_age`：航迹存活最大周期。
- `sim.data_path`：模拟器场景文件路径。

## 许可证

本项目遵循 MIT 许可证。
