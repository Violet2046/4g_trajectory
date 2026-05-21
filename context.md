# 项目上下文 — 4G Trajectory

> 生成日期: 2026-05-21
> IDE: VS Code + ESP-IDF v6.0
> 目标芯片: ESP32-C3
> 构建系统: CMake / Ninja

---

## 1. 项目概述
本项目为一个基于 ESP32-C3 平台的 4G 轨迹追踪系统。主要利用 CT511N (4G+GPS 模块) 获取并发送定位数据，同时利用 BMI160 传感器获取设备的运动姿态及加速度信息，用于后续的轨迹姿态融合分析及低功耗唤醒管理。

新增功能：通过 BLE 接收手机传输的图像，存入 Flash 后半区，并在 ST7789 显示屏（240×240 RGB565）上显示。图像接收期间传感器采样不中断（RAM 缓存临时存储）。

## 2. 核心模块与技术栈

### 2.1 主控芯片与环境
- **主控芯片**: ESP32-C3 (RISC-V 核心)
- **开发框架**: ESP-IDF v6.0。严格遵守最新 V6 API 开发底层外设驱动。
- **系统架构**: FreeRTOS。采用 Task, Mutex 和 vTaskDelay 等机制实现并发与非阻塞系统。
- **蓝牙栈**: NimBLE (ESP-IDF 内置)

### 2.2 CT511N 模块 (4G & GPS)
- **休眠控制**: 使用 dtr_pin，支持 `ct511n_sleep_dtr_enable()` 进行硬件级低功耗唤醒。

### 2.3 BMI160 模块 (六轴 IMU)
- **通信接口**: I2C (ESP-IDF v6 `i2c_new_master_bus`)。
- **中断控制**: INT1 引脚配置为上升沿触发，绑定 **AnyMotion** 运动唤醒。
- **Light-sleep 唤醒**: GPIO 9 同时配置为 Light-sleep 唤醒源。
- **Deep-sleep 限制**: GPIO 9 不是 RTC GPIO，仅靠 RTC 定时器唤醒。

### 2.4 AK09911C 模块 (3轴磁力计)
- **通信接口**: I2C，共享总线。

### 2.5 W25Q64 模块 (64M-bit NOR Flash) — 已启用
- **通信接口**: SPI (SPI2_HOST, Mode 0)，共享总线。
- **容量**: 8 MB，分为两个 4MB 分区：
  - **前半区** (`0x000000`–`0x3FFFFF`): 传感器采样数据环形缓冲区
  - **后半区** (`0x400000`–`0x7FFFFF`): 图像缓存区 (JPEG/RGB565)
- **引脚**: CS=GPIO1, SCK=GPIO2, MOSI=GPIO3, MISO=GPIO4
- **当前状态**: 已在 `hardware_init()` 中启用并初始化，`storage_mgr` 提供双分区 API。

### 2.6 ST7789 显示模块 (240×240 RGB LCD)
- **通信接口**: SPI (SPI2_HOST, Mode 3)，共享总线，40 MHz
- **像素格式**: RGB565 (16-bit, 每像素 2 字节)
- **引脚**: CS=GPIO11, DC=GPIO12, RST=GPIO13, BLK=GPIO14
- **功能**: 全屏填充、窗口绘制、从 Flash 图像区直接读取并显示

### 2.7 BLE 图像接收模块
- **协议**: NimBLE GATT Server，自定义 128-bit UUID 服务
- **特征**:
  - `DATA` (Write): 接收图像数据块 (最大 512 字节/包)
  - `CTRL` (Write): 控制命令 (START/DATA/DONE/CANCEL/DISPLAY)
- **控制协议**:
  - `IMG_CMD_START` (0x01): 启动传输，后跟 4 字节总大小
  - `IMG_CMD_DONE` (0x03): 传输完成，触发自动显示
  - `IMG_CMD_DISPLAY` (0x05): 重新显示上一张图片
- **图像格式**: 原始 RGB565 数据 (240×240 = 115,200 字节)

### 2.8 主程序工作流（当前状态机）

```
INIT → ACTIVE → SAMPLE → ACTIVE
         ↕ (5s无运动)   ↕ (BOOT键)
     LOW_POWER       IMG_RECEIVE
         ↓ (30min)
     DEEP_SLEEP
```

#### 状态说明

| 状态 | 行为 |
|------|------|
| **STATE_INIT** | `hardware_init()`: 初始化 CT511N、sensor_hub、W25Q64、gptimer、ISR、BOOT按键 |
| **STATE_ACTIVE** | 等待 1s 定时器采样、AnyMotion 中断、或 BMI160 双击 (INT2 → IMG_RECEIVE) |
| **STATE_LOW_POWER** | 传感器休眠、CPU Light-sleep、30min 后备 Deep-sleep |
| **STATE_SAMPLE** | 唤醒 CT511N → GPS → 传感器 → 存 Flash (或 RAM 缓存) → 上传 |
| **STATE_IMG_RECEIVE** | BLE 广播 → 手机连接 → 接收图像写入 Flash 后半区 → 自动显示 → 退出 |
| **STATE_DEEP_SLEEP** | `esp_deep_sleep_start()` (RTC 定时器唤醒) |

## 3. 文件结构

```
4g_trajectory/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt          # REQUIRES: ... ST7789 ble_img_rx
│   ├── main.c                  # 状态机 (~350行)
│   ├── app_utils.h/.c          # 工具函数 + IMG_RECEIVE 辅助
│   ├── storage_mgr.h/.c        # 双分区 + RAM Cache
├── components/
│   ├── CT511N/                 # 4G+GPS (UART)
│   ├── BMI160/                 # 6轴 IMU (I2C)
│   ├── AK09911C/               # 3轴磁力计 (I2C)
│   ├── W25Q64/                 # NOR Flash (SPI, 已启用)
│   ├── sensor_hub/             # 传感器管理中间层
│   ├── ST7789/                 # 240×240 LCD 驱动 (SPI) ★ 新增
│   └── ble_img_rx/             # BLE 图像接收服务 (NimBLE) ★ 新增
```

## 4. 开发阶段排雷记录

### 早期阶段
1. **CT511N AT 指令粘包死锁**: UART 未回复完即发新指令导致模块异常。
2. **CIPSEND 逗号崩溃**: 经纬度逗号 → 空格替换。
3. **Legacy I2C 重构**: 改用 ESP-IDF v6 `i2c_new_master_bus`。
4. **INT_MAP 语义修正**: `INT_MAP[0]=0x04` (bit2=AnyMotion)。
5. **ESP-IDF v6 API 适配**: `gpio_isr_t`、`esp_driver_i2c`+`esp_driver_gpio`。

### 低功耗增强
6. **Light-sleep + Deep-sleep**: GPIO 9 唤醒 Light-sleep，30min Deep-sleep 后备。
7. **修复弃用警告**: `esp_sleep_get_wakeup_cause()` → `esp_sleep_get_wakeup_causes()`。

### BLE 图像功能 (2026-05-21 新增)
8. **ST7789 显示驱动创建**: 新组件 `components/ST7789`，SPI Mode 3，40 MHz。
9. **BLE 图像接收服务创建**: 新组件 `components/ble_img_rx`，基于 NimBLE GATT Server。
10. **W25Q64 启用 + 双分区**: Flash 前半区 (4MB) 传感器数据，后半区 (4MB) 图像缓存。
11. **RAM 缓存缓冲**: `RAM_CACHE_CAPACITY=512` 条记录循环缓冲，`storage_set_ram_mode()` 切换。
12. **状态机扩展**: 新增 `STATE_IMG_RECEIVE`，BOOT 按键 (GPIO 0)触发进入。
13. **SPI 总线共享**: W25Q64 与 ST7789 共享 SPI2_HOST，分 CS 引脚分时复用。
14. **并行采样**: IMG_RECEIVE 期间传感器采样不中断，数据写入 RAM 缓存，退出时批量刷回 Flash。

## 5. 项目规范

- **ESP-IDF v6 API 优先**
- **CMake 精确依赖**: REQUIRES 精确列出
- **零警告编译**
- **驱动层分离**: 每个外设独立组件
- **英文注释**
- **main.c 纯状态机**
- **NULL 保护**
- **更新策略**: 每次对话后更新本文档

### 配置要求 (BLE)
在 `idf.py menuconfig` 中启用:
- `Component config → Bluetooth → Bluetooth → NimBLE`
- `Component config → Bluetooth → Bluetooth → Bluetooth Host → NimBLE`
## 4. 开发阶段排雷记录

### 早期阶段
1. **CT511N AT 指令粘包死锁**: UART 未回复完即发新指令导致模块异常。
2. **CIPSEND 逗号崩溃**: 经纬度逗号 → 空格替换。
3. **Legacy I2C 重构**: 改用 ESP-IDF v6 `i2c_new_master_bus`。
4. **INT_MAP 语义修正**: `INT_MAP[0]=0x04` (bit2=AnyMotion)。
5. **ESP-IDF v6 API 适配**: `gpio_isr_t`、`esp_driver_i2c`+`esp_driver_gpio`。

### 低功耗增强
6. **Light-sleep + Deep-sleep**: GPIO 9 唤醒 Light-sleep，30min Deep-sleep 后备。
7. **修复弃用警告**: `esp_sleep_get_wakeup_cause()` → `esp_sleep_get_wakeup_causes()`。

### BLE 图像功能 (2026-05-21)
8. **ST7789 显示驱动**: `components/ST7789`，SPI Mode 3，40 MHz，240×240 RGB565。
9. **BLE 图像接收**: `components/ble_img_rx`，NimBLE GATT Server。
10. **W25Q64 启用 + 双分区**: Flash 前 4MB 传感器数据，后 4MB 图像缓存。
11. **RAM 缓存机制**: `RAM_CACHE_CAPACITY=512`，`storage_set_ram_mode()` 切换。
12. **SPI 总线共享**: W25Q64 + ST7789 共享 SPI2_HOST，分时复用。
13. **并行采样**: IMG_RECEIVE 期间传感器数据 → RAM 缓存，退出时刷回 Flash。

### 双击唤醒 IMG_RECEIVE (2026-05-21)
14. **BMI160 双击检测代替 BOOT 按键**: 
    - 新增 `bmi160_double_tap_configure()` → 使能 INT_EN_2 双击位 + 映射到 INT2 (INT_MAP_1 bit6)
    - BMI160 INT2 引脚 = GPIO 0，配置为 `GPIO_INTR_POSEDGE`
    - `bmi160_int2_isr_add()` 注册 `bmi160_double_tap_isr()`，置位 `g_img_trigger_flag`
    - STATE_ACTIVE 检测标志 → 跳转 `STATE_IMG_RECEIVE`
    - `sensor_hub_sleep()` 中保留 INT2 映射（`BMI160_INT_MAP_DOUBLE_TAP`）
    - `sensor_hub_t` 增加 `double_tap_enabled` 布尔标志
    - 移除了原来的 GPIO 0 BOOT 按键 ISR 相关代码

## 5. 项目规范

- **ESP-IDF v6 API 优先**
- **CMake 精确依赖**
- **零警告编译**
- **驱动层分离**
- **英文注释**
- **main.c 纯状态机**
- **NULL 保护**
- **更新策略**: 每次对话后更新本文档

> **BLE 配置**: 需在 `idf.py menuconfig` 中启用 `Component config → Bluetooth → Bluetooth → NimBLE`