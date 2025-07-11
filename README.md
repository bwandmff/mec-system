# MEC设备软件系统

这是一个基于Linux C语言开发的MEC（移动边缘计算）设备软件系统，用于处理多传感器数据并进行目标检测、跟踪和数据融合。

## 系统架构

系统包含三个核心模块：

1. **视频分析与预处理模块** (`src/video/`)
   - 支持RTSP协议的多路摄像机接入
   - 实时目标检测与跟踪
   - 坐标系透视变换（图像坐标转WGS84坐标）
   - 可配置的检测区域管理
   - 障碍物检测功能

2. **雷达数据预处理模块** (`src/radar/`)
   - 毫米波雷达数据接收与解析
   - 串口通信管理
   - 数据格式转换和结构化处理
   - 极坐标到直角坐标转换

3. **数据融合模块** (`src/fusion/`)
   - 多传感器数据关联与融合
   - 卡尔曼滤波器实现
   - 目标航迹预测与更新
   - 统一的目标航迹输出

## 项目结构

```
mec_system/
├── CMakeLists.txt          # CMake构建配置
├── build.sh               # 构建脚本
├── include/               # 头文件
│   ├── mec_common.h       # 通用定义和数据结构
│   ├── mec_video.h        # 视频处理模块接口
│   ├── mec_radar.h        # 雷达处理模块接口
│   └── mec_fusion.h       # 数据融合模块接口
├── src/
│   ├── main.c             # 主程序入口
│   ├── common/            # 通用功能模块
│   │   ├── config.c       # 配置文件管理
│   │   ├── logging.c      # 日志系统
│   │   ├── memory.c       # 内存管理
│   │   ├── thread.c       # 线程管理
│   │   └── track_list.c   # 航迹列表管理
│   ├── video/             # 视频处理模块
│   │   └── video_processor.cpp
│   ├── radar/             # 雷达处理模块
│   │   └── radar_processor.c
│   └── fusion/            # 数据融合模块
│       └── fusion_processor.c
└── config/
    └── mec.conf           # 系统配置文件
```

## 依赖库

- OpenCV 4.x (用于视频处理)
- pthread (线程支持)
- 标准C库

## 构建和安装

1. 安装依赖：
```bash
# Ubuntu/Debian
sudo apt-get install cmake build-essential libopencv-dev

# CentOS/RHEL
sudo yum install cmake gcc-c++ opencv-devel
```

2. 构建项目：
```bash
./build.sh
```

3. 安装系统服务（可选）：
```bash
sudo make install
```

## 配置

系统配置文件位于 `config/mec.conf`，主要配置项包括：

- 视频流配置（RTSP地址、分辨率、帧率等）
- 雷达设备配置（串口路径、波特率、分辨率等）
- 数据融合参数（关联阈值、置信度阈值等）
- 透视变换参数（需要现场标定）
- 检测区域配置

## 运行

```bash
# 运行主程序
./build/mec_system

# 或者使用配置文件
./build/mec_system -c /path/to/config/mec.conf
```

## 系统特性

### 内存管理
- 自定义内存分配器，支持内存泄漏检测
- 统一的内存管理接口

### 多线程处理
- 每个模块独立线程处理，保证实时性
- 线程安全的数据共享机制
- 优雅的线程启动和停止

### 错误处理与日志
- 分级日志系统（DEBUG、INFO、WARN、ERROR）
- 线程安全的日志记录
- 详细的错误信息和调试信息

### 配置管理
- 灵活的配置文件格式
- 支持运行时配置重载
- 类型安全的配置访问接口

## 开发说明

### 添加新传感器

1. 在 `include/` 目录创建传感器头文件
2. 在 `src/` 目录创建对应的源文件
3. 实现传感器数据接口，输出标准的 `target_track_t` 格式
4. 在主程序中集成新传感器

### 坐标系标定

透视变换矩阵需要通过现场标定获得：

1. 在视频画面中标记已知WGS84坐标的控制点
2. 使用最小二乘法计算变换矩阵
3. 将矩阵参数写入配置文件

### 性能优化

- 使用 `-O2` 编译优化
- 关键路径避免动态内存分配
- 合理设置线程优先级
- 使用SIMD指令优化计算密集型操作

## 维护和监控

- 日志文件：`/var/log/mec_system.log`
- 配置文件：`/etc/mec/mec.conf`
- 进程监控：支持systemd服务管理
- 性能指标：可通过日志查看处理帧率和延迟

## 许可证

本项目遵循MIT许可证。