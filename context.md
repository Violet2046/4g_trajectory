# 项目上下文 — 4G Trajectory

> 生成日期: 2026-04-27
> IDE: VS Code + ESP-IDF v6.0
> 目标芯片: ESP32-C3
> 构建系统: CMake / Ninja

---

## 1. 项目概述
本项目为一个基于 ESP32-C3 平台的 4G 轨迹追踪系统。主要利用 CT511N (4G+GPS 模块) 获取并发送定位数据，同时利用 BMI160 传感器获取设备的运动姿态及加速度信息，用于后续的轨迹姿态融合分析及低功耗唤醒管理。

## 2. 核心模块与技术栈

### 2.1 主控芯片与环境
- **主控芯片**: ESP32-C3 (RISC-V 核心)
- **开发框架**: ESP-IDF v6.0。严格遵守最新 V6 API 开发底层外设驱动 (如 I2C/UART/GPIO)。
- **系统架构**: FreeRTOS。采用 Task, Mutex 和 vTaskDelay 等机制实现并发与非阻塞系统。

### 2.2 CT511N 模块 (4G & GPS)
- **休眠控制**: 使用 dtr_pin，支持 ct511n_sleep_dtr_enable() 进行硬件级低功耗唤醒。
- **业务逻辑**:
  - 非阻塞式 AT 轮询和重叠阻断机制。
  - GPS 快速获取（非阻塞等待 \）。
  - Payload 格式转化：去除经纬度字符中的逗号转分为号，规避 CIPSEND 发送时导致的模块解析崩溃。

### 2.3 BMI160 模块 (六轴 IMU)
- **通信接口**: I2C (ESP-IDF v6 i2c_new_master_bus)。
- **中断控制**: INT1 引脚配置为上升沿触发 (GPIO_INTR_POSEDGE)，绑定 Significant-Motion 运动唤醒机制。
- **传感器配置**: 加速度 8g / 100Hz ODR，陀螺仪 500dps / 100Hz ODR。
- **ISR 注册**: `bmi160_int1_isr_add()` 内部调用 `gpio_install_isr_service` + `gpio_isr_handler_add`。
- **中断映射**: INT_MAP[0] bit2=0x04 → 路由 SigMotion 到 INT1。
- **INT_OUT_CTRL**: 0x0A → int1_output_en=1, int1_lvl=1 (active high), push-pull。

### 2.4 AK09911C 模块 (3轴磁力计)
- **通信接口**: I2C (ESP-IDF v6 i2c_master_bus)。
- **从机地址**: 0x0C (CAD=0) 或 0x0D (CAD=1)，由硬件 CAD 引脚决定。
- **灵敏度**: 0.6 µT/LSB，14-bit 数据输出（16-bit two's complement），量程 ±4900 µT。
- **校准**: 出厂熔丝 ROM 存储每轴灵敏度调整系数 (ASAX/Y/Z)，`ak09911_init()` 自动读取并存储。
- **读数模式**: 支持单次测量、连续测量 1–4 (10/20/50/100Hz)、自检、熔丝 ROM 访问。
- **数据读取**: 轮询 DRDY 位 → burst-read 8 bytes (HXL–ST2)；ST2 必须读以释放数据保护。
- **模式切换**: 必须先回到 Power-down 模式，等待 ≥100µs (Twat) 再设新模式。

### 2.5 W25Q64 模块 (64M-bit NOR Flash)
- **通信接口**: Standard SPI (ESP-IDF v6 spi_master)，Mode 0 (CPOL=0, CPHA=0)。
- **容量**: 8 MB (32,768 pages × 256 bytes)，支持 4KB/32KB/64KB 擦除及全片擦除。
- **JEDEC ID**: 制造商 0xEF (Winbond)，设备 ID 0x4017 (IQ/JQ 变体)。
- **基本操作**: Write Enable (06h) → Page Program (02h, ≤256B) / Erase → 轮询 BUSY 位。
- **状态寄存器**: SR1 (BUSY/WEL/BP/TB/SEC/SRP)、SR2 (SRL/QE/LB/CMP/SUS)、SR3 (DRV/WPS)。
- **软件复位**: 66h (Enable Reset) + 99h (Reset Device)，约 30µs 完成。
- **低功耗**: Power-down (B9h) 后待机电流 <1µA，Release Power-down (ABh) 唤醒。

### 2.6 主程序工作流
1. **初始化阶段**: 启动 CT511N 模块 → 初始化 BMI160 (配置传感器 + sig-motion 中断 + 注册 ISR) → CT511N 进入 DTR 休眠。
2. **主循环**: 阻塞等待 BMI160 sig-motion 中断信号量 → 通过 DTR 唤醒 CT511N → TCP 连接 → GPS 初始化 → 读取 BMI160 (g + dps) → 构建 payload (GPS + BMI160 数据) → TCP 发送 → CT511N 再次休眠。

## 3. 开发阶段排雷记录
1. 修复了 CT511N UART 未回复完即发新指令的粘包死锁问题。
2. 修复了 CIPSEND 数据中带逗号引起发送中断的问题。
3. 主心跳循环大幅提速（消除死等）。
4. 修正了前期错误依赖 ESP-IDF legacy I2C 的问题，全面重构为 V6 master_bus。
5. **INT_MAP 映射语义修正**: 之前误以为 "0=INT1,1=INT2"，实际为 "set bit = 路由到对应引脚寄存器"。改为 INT_MAP[0]=0x04 (bit2=AnyMotion/SigMotion)。
6. **INT_OUT_CTRL 值修正**: 0x08→0x0A，int1_lvl 从 active low 改为 active high，匹配 ESP32 POSEDGE 触发。
7. **ESP-IDF v6 API 适配**: `gpio_isr_handler_t` 重命名为 `gpio_isr_t`，CMake REQUIRES 改为 `esp_driver_i2c` + `esp_driver_gpio`。
8. **未初始化变量修复**: `bmi160_acc_set_mode`/`bmi160_gyr_set_mode` 的 SUSPEND 分支补上 `expected` 赋值。
9. **AK09911C 驱动新增**: 3轴磁力计 I2C 驱动，参考 BMI160 模式，支持出厂校准系数自动读取、模式切换、burst 读数、灵敏度调整。
10. **agent.instructions.md 重写**: 从通用占位符升级为项目实际规范，新增 ESP-IDF v6 API 优先、CMake 精确依赖、中断寄存器对照手册、零警告编译、驱动层分离、代码风格英文注释等 8 条规则。
11. **AK09911C I2C 驱动创建**: 新建 `components/AK09911C` 组件（.h / .c / CMakeLists.txt），遵循 BMI160 驱动模式：opaque handle、reg_read/write 封装、芯片 ID 校验、工厂校准系数读取。
12. **W25Q64 SPI NOR Flash 驱动新增**: 新建 `components/W25Q64` 组件，基于 SPI master (Mode 0)。支持标准指令：Read Data (03h)、Page Program (02h)、Sector Erase (20h)、Block Erase (52h/D8h)、Chip Erase (C7h)。集成 JEDEC ID 校验 (EFh/4017h)、状态寄存器读写、BUSY 轮询、软件复位及 Power-down 低功耗。遵循 opaque handle 模式，与 BMI160/AK09911C 风格统一。
13. **主程序重构（状态机 + 定时器中断 + ROM 存储）**: 
    - 新增 `main/storage_mgr.h/.c`：W25Q64 存储管理层。ROM Pages 0-1 配置，Pages 2-7 索引日志（journal），Pages 16+ 循环数据记录（扇区对齐擦除）。
    - 重写 `main/main.c`：`sys_state_t` 状态机（ACTIVE→SAMPLE→UPLOAD→LOW_POWER→WAKING）。
    - **低功耗策略**：BMI160 AnyMotion 取代 SigMotion，无运动 >5s 自动进入 LOW_POWER——gptimer 停、CT511N 休眠、W25Q64 断电、BMI160 陀螺仪挂起/加速度计低功耗（仍可检测 anymotion）、AK09911C 断电。anymotion ISR 触发 WAKING 流程恢复全部模块。
    - 默认采样间隔 1s。采样时先唤醒 CT511N 取 GPS，再读 IMU/Mag，存 ROM；随后机会性检查 4G 信号——若有则复用已唤醒模块上传未传记录。
14. **主程序 v2 低功耗升级**: 采样间隔改为 1s，AnyMotion 唤醒替代 SigMotion，新增 inactivity timer（5s 无运动→全模块休眠），LOW_POWER 状态自动管理各模块电源，anymotion ISR 一键恢复。
15. **共享 I2C 总线重构 + 地址回退**: 
    - ESP32-C3 仅一个 I2C 控制器，BMI160 和 AK09911C 此前各自创建独立总线导致 AK09911C 初始化失败（"no free bus"）。重构为 main.c 先创建一条共享 I2C 总线，通过 `.bus_handle` 字段传入两个驱动。
    - BMI160 和 AK09911C 驱动增加 `bus_handle` 可选字段 + `bus_owned` 标志（外部总线不销毁）。
    - BMI160 增加地址回退 `0x68`→`0x69`（SDO 拉高时地址为 0x69）。
    - AK09911C 增加地址回退 `0x0C`→`0x0D`（CAD 拉高时地址为 0x0D），未找到时降级跳过而非 halt。
    - 所有涉及 g_ak09911 的调用（采样、低功耗、唤醒）均增加 NULL 保护。
16. **W25Q64 SPI Flash 暂时禁用 + 各模块降级保护**:
    - 硬件未连接 SPI Flash 时，`spi_bus_initialize()` 在 ESP32-C3 上会导致芯片崩溃（内部断言/panic），即使检查返回值也无救。因此暂时完全跳过 W25Q64 初始化。
    - `g_w25q64 == NULL` 时，Storage Manager 自动跳过，所有 `storage_*` 调用安全返回 0 或 false。
    - `low_power_enter/exit` 中所有 `w25q64_power_down/release` 均增加 NULL 保护。
    - 定时器默认间隔改为 1s（不受 storage 配置影响）。
    - ESP-IDF v6 中 `spi_device_interface_config_t` 新增 `clock_source`、`duty_cycle_pos` 等字段，必须显式初始化，否则使用栈垃圾值可能导致断言失败。
    - `spi_bus_config_t` 新增 `flags` 字段，需设置 `SPICOMMON_BUSFLAG_MASTER`。
    - ESP32-C3 强制要求 SPI DMA 自动分配，`dma_chan` 必须设为 `SPI_DMA_CH_AUTO`(3)，`-1`(无 DMA) 不被支持。
17. **sensor_hub 组件创建 + main.c 重构**:
    - 新建 `components/sensor_hub`：封装共享 I2C 总线管理、BMI160 自动地址回退（0x68/0x69）、AK09911C 自动地址回退（0x0C/0x0D）、传感器采样统一接口、低功耗休眠/唤醒管理。
    - main.c 精简约 50%：移除 BMI160/AK09911C 直接引用、移除共享 I2C 总线创建代码、移除调试标记、移除 ACCEL 调试日志；改用 `sensor_hub_init/sample/sleep/wake` 四函数驱动所有传感器。
    - 新增 `sensor_sample_t` 统一数据结构（时间戳/GPS/加速度/陀螺仪/地磁），`sampple_sensors()` 使用 `sensor_hub_sample()` 填充后写入 storage。
    - 状态机保留不变（ACTIVE→SAMPLE→UPLOAD→LOW_POWER），`low_power_enter/exit` 改调 `sensor_hub_sleep/wake`。
    - CMake 依赖链：main→sensor_hub→BMI160+AK09911C。
18. **第二次架构整理：app_utils 提取 + main.c 化简为纯状态机**:
    - 新建 `main/app_utils.h/.c`：提取 timer 启停、低功耗进入/退出、采样+日志（每 2s 压缩输出）、上传连接/发送等约 200 行辅助代码。
    - `main/main.c` 化简至约 180 行：仅保留 ISR、`hardware_init`、状态机循环三个模块。状态机中低功耗/采样/上传全部调用 `app_utils` 函数（传入句柄参数，不使用全局变量引用）。
    - 日志策略：`sample_sensors()` 每 2s（偶数次采样）输出完整摘要及 GPS 原始数据，奇数次仅静默存储，大幅减少串口日志洪泛。
    - 工作区文件结构保持精简：
      ```
      main/         → main.c, app_utils.h/.c, storage_mgr.h/.c, CMakeLists.txt
      components/   → CT511N, BMI160, AK09911C, W25Q64, sensor_hub
      ```
    - 所有复用性高的初始化逻辑已在早期移入各自组件（BMI160、AK09911C、sensor_hub），`main.c` 完成"胶水代码"角色。
19. **采样与上传联动 + 存储状态追踪**:
    - 合并之前的 SAMPLE 和 UPLOAD 状态机——STATE_SAMPLE 采样 → 存储到 Flash → 立即联网上传 → 刷新记录状态。
    - `upload_send_all()` 扫描 Flash 中所有 `STORAGE_REC_STATUS_WRITTEN(0xFE)` 记录逐条发送：成功标记 `UPLOADED(0xFC)`，失败标记 `FAILED(0x00)`，不因失败跳出循环。
    - 新增 `storage_record_set_last_status()` + `STORAGE_REC_STATUS_FAILED(0x00)`，通过 NOR 位翻转更新状态字节，无需整页擦除。
    - 无 Flash 时使用 RAM 缓存 `g_last_rec` 作为后备。
    - Payload 格式去除逗号（防止 CT511N CIPSEND 解析失败），改为空格分隔：`T:123 F:1 LA:39.9 ...`
20. **TCP 连接下沉到 upload_send_all 内部**:
    - 移除 `STATE_SAMPLE` 中独立的 `ct511n_tcp_single_connect()` 调用，改为 `upload_send_all()` 内部自己管理 TCP 连接。
    - `upload_send_all()` 流程： 第一时间沿用原本的tcp连接 -> 逐个发送待传记录 → 发送失败时自动重连一次 + 重试 → 仍然失败才标记 `FAILED(0x00)` → 继续处理下一条。
    - 移除了不再使用的 `upload_connect()` 函数，`app_utils.h` 只暴露 `upload_send_all()`。
    - 每次采样后无需外部判断是否连网，`upload_send_all()` 内部统一处理连接+发送+重试+失败标记。

## 4. 后续注意事项
