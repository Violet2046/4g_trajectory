# 项目上下文 — 4G Trajectory

> 更新日期: 2026-06-05
> IDE: VS Code + ESP-IDF v6.0
> 目标芯片: ESP32-C3
> 构建系统: CMake / Ninja

---

## 1. 项目概述

本项目为基于 ESP32-C3 的 4G 轨迹追踪系统，核心功能：

- **4G+GPS 定位上传**: CT511N 模块获取 GPS 坐标，通过 4G TCP（或 WiFi）上传到服务器
- **传感器采集**: BMI160 (6轴 IMU) + AK09911C (3轴磁力计)，3 秒采样周期
- **数据存储**: W25Q64 NOR Flash (8MB)，前 4MB 存传感器记录，后 4MB 存 BLE 图像
- **墨水屏显示**: QYEG0397 (800×480 4色)，右下角实时叠加传感器数据；双击触发 BLE 图像接收 & 全屏显示
- **低功耗**: PM tickless-idle 自动 light sleep，BMI160 any-motion 唤醒，采样间关闭磁力计和陀螺仪，**LOW_POWER 时停止 BLE 广播**
- **WiFi 配网**: SoftAP `4G-Tracker`，无 STA 连接时自动关闭（省电），每 10s 扫描重连；双击 BMI160 启动配网 AP（20s 超时）
- **BLE 图像**: NimBLE GATT Server，init 在 WiFi 之前（避堆碎片），空闲停止广播/运动恢复，`"4G-Tracker"`

## 2. 核心模块与技术栈

### 2.1 主控芯片与环境
- **主控芯片**: ESP32-C3 (RISC-V)
- **开发框架**: ESP-IDF v6.0，使用 `esp_driver_*` 新 API
- **系统架构**: FreeRTOS，单线程状态机 + ISR + 信号量
- **蓝牙栈**: NimBLE (编译时 `#ifdef CONFIG_BT_NIMBLE_ENABLED` 保护)

### 2.2 CT511N 模块 (4G & GPS)
- **通信接口**: UART1 (TX=5, RX=6), 115200 bps
- **休眠控制**: DTR 引脚 (GPIO 11) 硬件休眠
- **功能**: GPS 坐标、4G CCLK 授时、TCP 数据上传

### 2.3 BMI160 模块 (六轴 IMU)
- **通信接口**: I2C (ESP-IDF v6 `i2c_new_master_bus`, SDA=7, SCL=10)
- **中断控制**: INT1 (GPIO 20) → AnyMotion 运动唤醒；INT2 (GPIO 22) → 双击 → IMG_RECEIVE
- **低功耗**: 采样间陀螺仪 SUSPEND，加速度计保持 NORMAL

### 2.4 AK09911C 模块 (三轴磁力计)
- **通信接口**: 共享 I2C 总线 (地址 0x0C/0x0D)
- **模式**: 连续 10Hz，采样间 POWERDOWN 省电 (~1mA)

### 2.5 W25Q64 NOR Flash
- **通信接口**: SPI2_HOST, Mode 3, CS=1, 2MHz (面包板稳定)
- **兼容**: Winbond (0xEF) + Micron (0x20) JEDEC ID
- **分区**: 前 4MB 传感器循环缓冲区 + 后 4MB 图像缓存

### 2.6 QYEG0397 EPD 墨水屏
- **型号**: QYEG0397RYS677F3，4 色 (黑/白/红/黄)，800×480
- **控制器**: Solomon SSD1680 兼容，OTP 波形
- **接口**: SPI2_HOST 共享，Mode 0, CS=19, DC=20, RST=21, BUSY=0
- **帧缓冲**: 不使用 96KB RAM 缓冲。覆盖层用 10KB 静态数组 (`g_overlay_buf`) 渲染；全屏图像从 W25Q64 Flash 分段读取 (4KB chunk) 直接送 EPD。
- **EPD 引脚**: CS=15, DC=16, RST=17, BUSY=18（`config.h` 中 `CFG_EPD_*`）
- **EPD 驱动**:
  - `epd_update_partial_window()` — 紧凑窗口缓冲（仅覆盖层区域），无需全帧缓冲
  - `epd_update_full_from_flash()` — 全屏刷新，从 Flash 分段读取 4KB 块送 EPD
  - `epd_sensor_overlay_window()` — 传感器叠加层在 10KB 窗口缓冲上绘制
- **功能**: 右下角 264×160 传感器叠加面板 (部分刷新 ~4-6s) + BLE 图像全屏显示 (~18s)

### 2.7 WiFi 配网 (wifi_cfg)
- **SoftAP**: SSID `4G-Tracker`, 密码 `12345678`
- **配置页**: `http://192.168.4.1`，设置服务器 IP/端口/WiFi STA
- **SNTP**: STA 连接后自动同步 `ntp.aliyun.com`

### 2.8 主程序状态机
```raw
INIT → ACTIVE → SAMPLE → ACTIVE (EPD 叠加更新)
         ↕ (5s无运动)   ↕ (双击)
     LOW_POWER       IMG_RECEIVE (EPD 全屏显示)
```
#### 状态说明

| 状态 | 行为 |
|------|------|
| **STATE_INIT** | `hardware_init()`: 初始化 CT511N、sensor_hub、W25Q64、gptimer、ISR、EPD |
| **STATE_ACTIVE** | BLE 广播（WiFi 关闭）。等待定时器采样、AnyMotion 中断、或 BMI160 双击 (INT2 → AP 配网)。每 5 分钟停 BLE 做一次 WiFi STA 扫描。 |
| **STATE_LOW_POWER** | 传感器休眠、PM tickless-idle 自动 light sleep、信号量 + 运动 ISR 唤醒。BLE 和 WiFi 均关闭。 |
| **STATE_SAMPLE** | 唤醒 CT511N → GPS → 传感器 → 存 Flash → 上传 (WiFi优先/4G回退) → EPD 叠加更新 |
| **STATE_IMG_RECEIVE** | BLE 广播 → 手机连接 → 接收图像写入 Flash 后半区 → EPD 全屏显示 → 退出 |

#### 上电顺序

```raw
hardware_init → EPD init
  → WiFi STA-only (5s, 无 BLE)  ← 干净堆
  → STA成功: upload_send_all
  → wifi_cfg_stop()             ← 释放 WiFi 内存
  → BLE init + 广播              ← BLE 独占全部堆
  → 主循环
```

#### 双击 BMI160 流程

```raw
BLE 暂停 (ble_img_pause, 释放 ~50KB)
  → WiFi AP-only (60s)
  → 等 20s 设备连接
  → 设备已连: 保持到配置保存或 60s 超时
  → 未连接: 20s 后退出
  → AP 关闭
  → BLE 恢复 (ble_img_resume)
```
```raw
4g_trajectory/
├── CMakeLists.txt
├── sdkconfig.defaults          # PM + tickless-idle + BLE 启用
├── main/
│   ├── CMakeLists.txt          # REQUIRES: ... nvs_flash epd_qyeg0397
│   ├── main.c                  # 状态机 + EPD 初始化 + IMG_RECEIVE(EPD)
│   ├── app_utils.h/.c          # EPD app + 传感器省电 + BLE 回调
│   ├── config.h                # 所有引脚/时序/网络 单一定义
│   ├── storage_mgr.h/.c        # Flash 存储 双分区 + RAM Cache
├── components/
│   ├── epd_qyeg0397/           # QYEG0397 墨水屏 (800×480 4色)
│   │   ├── CMakeLists.txt
│   │   ├── epd_qyeg0397.c      # SSD1680 驱动 + OTP 波形
│   │   ├── epd_font.c          # 8×16 字体 + 文本渲染
│   │   └── include/
│   ├── CT511N/                 # 4G+GPS (UART)
│   ├── BMI160/                 # 6轴 IMU (I2C)
│   ├── AK09911C/               # 3轴磁力计 (I2C)
│   ├── W25Q64/                 # NOR Flash (SPI)
│   ├── sensor_hub/             # 传感器管理
│   ├── ble_img_rx/             # BLE 图像接收 (NimBLE GATT Server)
│   └── wifi_cfg/               # WiFi 配网 + TCP
│       ├── wifi_cfg.c          # SoftAP + HTTP + STA, 含 DMA 池 (osi_funcs hook)
│       └── include/wifi_cfg.h
```

---
> **以上为项目参考文档（概览、模块、文件结构）—— 修改工作区后请同步更新。**
> **以下为修改日志（Changelog），每一条记录仅追加、绝不更改。**
---

## 5. 修改日志 / Changelog

> ⚠️ 日志规则：每次修改后在此追加新条目，已写入的条目**不得修改或删除**，确保历史可追溯。

### 早期阶段
1. **CT511N AT 指令粘包死锁**: UART 未回复完即发新指令导致模块异常。
2. **CIPSEND 逗号崩溃**: 经纬度逗号 → 空格替换。
3. **Legacy I2C 重构**: 改用 ESP-IDF v6 `i2c_new_master_bus`。
4. **INT_MAP 语义修正**: `INT_MAP[0]=0x04` (bit2=AnyMotion)。
5. **ESP-IDF v6 API 适配**: `gpio_isr_t`、`esp_driver_i2c`+`esp_driver_gpio`。

### 低功耗增强
6. **Light-sleep + Deep-sleep**: GPIO 20（BMI160 INT1）唤醒 Light-sleep，30min Deep-sleep 后备。
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
    - `bmi160_double_tap_configure()` → INT_EN_2 双击 + INT_MAP_1 bit6 → INT2 GPIO 0
    - `bmi160_int2_isr_add()` → `g_img_trigger_flag`
    - `sensor_hub_sleep()` 保留 INT2 映射
    - 移除 GPIO 0 BOOT 按键 ISR

### 2026-06-01 批量问题修复
15. **W25Q64 SPI 半双工冲突**: `SPI_DEVICE_HALFDUPLEX` + 全双工 tx+rx 不兼容 → 移除标志 `flags=0`。
16. **BMI160 warm-reset 芯片ID 错误**: I2C 总线在 warm-reset 时被 BMI160 锁死（SDA 拉低）。修复：`sensor_hub_init()` 在创建 I2C 总线前手动发 9 个 SCL 脉冲 + STOP 恢复总线；`bmi160_create()` 软复位 + 3 次重试。
17. **LOW_POWER TWDT 看门狗 — 最终修复**:
    - `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=y` 在 boot 时自动订阅 IDLE 到 TWDT。
    - `esp_task_wdt_delete()` / `esp_task_wdt_deinit()` 在 sleep 唤醒路径上可能被 ESP-IDF 内部的 timer restore 逻辑覆盖重新初始化。
    - **终极方案**：sdkconfig 设 `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=n`，从硬件配置层面禁止 IDLE 被 TWDT 监控。代码层保留 `esp_task_wdt_deinit()` 作为双重保险。
    - 需 `idf.py fullclean && idf.py build` 确保配置生效。
18. **CT511N UART 响应污染**: `ct511n_send_at_command()` 发送前 `uart_flush_input()`。
19. **upload_send_all 未先建 TCP**: 循环前先 `ct511n_tcp_single_connect()`。
20. **gpio_install_isr_service 重复**: 静态标志 `s_gpio_isr_installed`。
21. **BMI160 Any-motion 唤醒修复**: 调整了 `INT_OUT_CTRL` 为 `0xAA` 以开启 INT1 和 INT2 双引脚输出。`LP_WAKEUP_GPIO` 设为 GPIO 20 与硬件一致。
22. **引脚配置统一管理**: 创建 `main/pin_config.h` 作为所有硬件引脚定义的单一入口，移除 `main.c` 和 `app_utils.h` 中的分散定义。`BMI160_INT1_PIN`、`BMI160_INT2_PIN`、`LP_WAKEUP_GPIO` 以及 SPI/I2C/UART 引脚全部集中管理。

### 2026-06-01 BMI160 I2C 超时故障排查
23. **BMI160 INT1 实际引脚修正**: 硬件 BMI160 INT1 实际连接 GPIO 20（而非之前文档记录的 GPIO 9）。`pin_config.h` 中 `BMI160_INT1_PIN` 保持为 20，`LP_WAKEUP_GPIO` 与其一致。
24. **BMI160 冷启动芯片 ID 读取失败（err=264 ESP_ERR_TIMEOUT）**:
    - **症状**: `bmi160_create()` 无法读取到正确的 chip_id (预期 `0xD1`)，读回 `0x00`、`0x20` 或 `0x1C`，I2C 返回 `ESP_ERR_TIMEOUT (264)`。
    - **修复**: `bmi160_create()` 中改为：无论首次读取是否成功，都先重置 I2C 外设 → 发送软复位命令 → 等待 50ms → 重置 I2C → 重试 3 次。
25. **BMI160.c 改进摘要**: 芯片 ID 验证流程简化为无条件 I2C 总线复位 → 无条件 BMI160 软复位 → 3 次重试读 chip_id。移除了条件分支逻辑。

### 2026-06-01 Light-sleep 唤醒修复
26. **`bmi160_any_motion_configure()` 寄存器写入顺序颠倒（关键BUG）**:
    - **根因**: `INT_MOTION_0` (0x5F) 是 anym_dur，`INT_MOTION_1` (0x60) 是 anym_thr，代码写反。
    - **修复**: 交换写入顺序。
27. **ISR 触发类型与 Sleep 唤醒类型不匹配 + API 弃用修复**:
    - **根因**: ISR 为 `GPIO_INTR_POSEDGE`，唤醒为 `GPIO_INTR_HIGH_LEVEL`，CPU 唤醒后 ISR 不触发 → `g_motion_flag` 为 false，依赖 I2C 轮询可能失败。
    - **修复**: 唤醒后检查 `esp_sleep_get_wakeup_causes()` 位掩码，若为 GPIO 唤醒直接退出 LOW_POWER。
28. **`sensor_hub_sleep()` 静默错误处理**: 所有 I2C 步骤检查返回值，失败时 `ESP_LOGE` 并返回。`low_power_enter()` 检查返回值打 `ESP_LOGW`。
29. **context.md 结构规范化**: 上方为项目参考文档（概览/模块/文件结构），下方为修改日志（Changelog）。日志条目仅追加、绝不更改。
### 2026-06-01 Light-sleep 唤醒修复

26. **`bmi160_any_motion_configure()` 寄存器写入顺序颠倒（关键BUG）**:
    - **根因**: BMI160 数据手册中 `INT_MOTION_0` (0x5F) 是 anym_dur（持续时间），`INT_MOTION_1` (0x60) 是 anym_thr（阈值）。但代码将 `threshold` 写入了 duration 寄存器、`duration` 写入了 threshold 寄存器——完全颠倒。
    - **影响**: 阈值被设为 0x01（~62.5mg，过于敏感），持续时间被设为 0x06（6 个采样点）。虽然功能上仍可能触发，但参数语义错误。
    - **修复**: 交换写入顺序，`duration` → `INT_MOTION_0`、`threshold` → `INT_MOTION_1`。

27. **ISR 触发类型与 Sleep 唤醒类型不匹配 + API 弃用修复**:
    - **根因**: BMI160 INT1 ISR 注册为 `GPIO_INTR_POSEDGE`（上升沿），但 `gpio_wakeup_enable()` 配置为 `GPIO_INTR_HIGH_LEVEL`（高电平）。CPU 从 Light-sleep 唤醒时，上升沿已发生但 ISR 不会触发 → `g_motion_flag` 保持 false → 只能依赖 I2C 轮询 `INT_STATUS` 兜底。若 I2C 不稳定，轮询失败 → 永远无法退出 LOW_POWER。
    - **修复**: 在 `STATE_LOW_POWER` 循环中，`esp_light_sleep_start()` 返回后检查 `esp_sleep_get_wakeup_causes()`（替代已弃用的 `esp_sleep_get_wakeup_cause`）。若包含 `ESP_SLEEP_WAKEUP_GPIO` 位，直接视为 BMI160 触发唤醒，退出 LOW_POWER。不再依赖 ISR 或 I2C 轮询来判定 GPIO 唤醒。

28. **`sensor_hub_sleep()` 静默错误处理**:
    - **根因**: 所有 I2C 操作（设模式、any-motion 配置、中断映射）的返回值均被忽略，错误被静默吞掉。
    - **修复**: 每个步骤检查返回值，失败时打印 `ESP_LOGE` 并立即返回错误。`low_power_enter()` 检查返回值并打印 `ESP_LOGW`。`low_power_sleep_configure()` 的日志从 `ESP_LOGD` 提升为 `ESP_LOGI` 以便调试。
### 2026-06-01 LOW_POWER 无任何唤醒 — 诊断日志追加

30. **LOW_POWER 状态进入后完全无输出**:
    - **症状**: 进入 `STATE_LOW_POWER` 后，日志停在 `light-sleep wakeup configured on GPIO 9`。晃动模块无任何反应，连 1 秒定时器唤醒也未触发——`esp_light_sleep_start()` 疑似未返回。
    - **诊断**: 在 `low_power_sleep_enter()` 前后分别添加 `ESP_LOGD`（进入前）和 `ESP_LOGI`（返回后打印 `esp_sleep_get_wakeup_causes()` 位掩码），用于判断 sleep 是否真正进入、以何种原因唤醒。

### 2026-06-01 `esp_light_sleep_start()` 永不返回 — 最小化隔离测试

31. **确认 `esp_light_sleep_start()` 本身卡死**:
    - **症状**: 日志停在 `entering light-sleep (gpio=9, timer=1s)…`，连 1 秒定时器唤醒也未返回 → sleep 入口本身有问题。
    - **隔离测试**: 将 `STATE_LOW_POWER` 替换为极简代码——仅 `esp_sleep_enable_timer_wakeup(2s)` + `esp_light_sleep_start()`，不做任何 GPIO 配置、BMI160 I2C 轮询。循环 5 次后自动退出。
    - **目的**: 判断是 GPIO/BMI160 干扰了 sleep，还是 `esp_light_sleep_start()` 在 ESP-IDF v6.0 + ESP32-C3 上根本不能正常工作。
### 2026-06-01 启用 `CONFIG_PM_ENABLE` 修复 `esp_light_sleep_start()` 卡死

32. **`esp_light_sleep_start()` 在无 GPIO/I2C 情况下仍卡死不返回**:
    - **根因**: ESP-IDF v6.0 中 `esp_light_sleep_start()` 可能依赖 Power Management 框架。当前 `sdkconfig` 中 `CONFIG_PM_ENABLE=n`，导致 sleep 入口无法正确处理 CPU 上下电。
    - **修复**: 设置 `CONFIG_PM_ENABLE=y`，执行 `idf.py fullclean build flash monitor` 确保重建。
### 2026-06-01 `sdkconfig.defaults` 持久化 PM_ENABLE

33. **手动编辑 `sdkconfig` 被构建系统覆盖**:
    - **根因**: `idf.py fullclean build` 会重新生成 `sdkconfig`，直接编辑会丢失。
    - **修复**: 创建 `sdkconfig.defaults` 文件，写入 `CONFIG_PM_ENABLE=y` 和 `CONFIG_FREERTOS_USE_TICKLESS_IDLE=y`。Kconfig 每次运行时会自动合并这些默认值，不会被覆盖。
### 2026-06-01 用 deep sleep 验证 RTC 定时器

34. **隔离测试——`esp_deep_sleep_start()`**:
    - 将 `STATE_LOW_POWER` 改为 `esp_deep_sleep_start()` + 5 秒定时器。
    - Deep sleep 后 CPU 会重启，如果 5 秒后看到 boot 信息 → RTC 定时器正常，问题出在 light sleep 的 CPU 上下电/外设恢复。
    - 移除了 `esp_task_wdt_deinit()` 调用，避免与 `esp_light_sleep_start()` 内部 TWDT 处理冲突。
### 2026-06-01 最终方案：信号量等待替代 light sleep

35. **Deep sleep RTC 定时器验证通过**: `esp_deep_sleep_start()` + 5s 定时器正常重启，RTC 域无问题。
36. **`esp_light_sleep_start()` 在 ESP-IDF v6.0 + ESP32-C3 上无法返回**（即使 `CONFIG_PM_ENABLE=y` 也无效）。
    - **最终方案**: 不依赖 `esp_light_sleep_start()`，改用 `xSemaphoreTake(g_motion_sem, 1s)` 阻塞等待。
    - **唤醒机制**: BMI160 INT1 ISR → `xSemaphoreGiveFromISR(g_motion_sem)` → 阻塞解除 → 退出 LOW_POWER → STATE_SAMPLE。
    - **兜底**: 1 秒超时后轮询 BMI160 `INT_STATUS`、检查 deep-sleep 超时。
    - **功耗说明**: CPU 未进入硬件 sleep，但保持 `CONFIG_PM_ENABLE=y` 在 sdkconfig 中，后续可通过 FreeRTOS tickless idle 进一步降低功耗。
### 2026-06-02 移除 Deep Sleep，简化为信号量 Light Sleep

37. **移除 Deep Sleep 模式**:
    - 删除 `STATE_DEEP_SLEEP` 状态、`DEEP_SLEEP_FALLBACK_S` 常量、`low_power_deep_sleep_enter()` 函数。
    - LOW_POWER 中移除 deep-sleep 超时回退逻辑。
    - 当前低功耗方案：`xSemaphoreTake(g_motion_sem, 1s)` 阻塞 + BMI160 ISR 唤醒。CPU 保持运行但不进入硬件 sleep。
### 2026-06-02 添加 2 秒防抖：两次运动间隔 ≥ 2s 才退出 LOW_POWER

38. **LOW_POWER 退出增加防抖机制**:
    - 不再一检测到运动就退出，而是要求**两次运动事件间隔 ≥ 2 秒**。
    - 第一次运动 → 记录时间戳，标记 hit #1。
    - 第二次运动 → 若距 hit #1 ≥ 2s → 退出 LOW_POWER；若 < 2s → 重置窗口（此次变为新的 hit #1）。
    - 若当前循环无运动 → 重置计数器，避免超时累积。
    - 三个检测源统一逻辑：信号量（ISR 驱动）、ISR flag、I2C 轮询 INT_STATUS。进入循环时先清空可能残留的信号量，避免同一事件被重复计数。
### 2026-06-02 会话总结

39. **当前低功耗方案（最终状态）**:
    - LOW_POWER 通过 `xSemaphoreTake(g_motion_sem, 1s)` 阻塞等待。
    - `CONFIG_PM_ENABLE=y` + `CONFIG_FREERTOS_USE_TICKLESS_IDLE=y`：idle task 自动进入硬件 light sleep，BMI160 INT1 → GPIO 唤醒 → ISR 释放信号量 → 任务恢复。
    - 防抖：两次运动间隔 ≥ 2s 才退出 LOW_POWER。第一次记录时间戳，< 2s 内的运动被忽略，≥ 2s 后的运动触发退出。
    - 移除 Deep Sleep（STATE_DEEP_SLEEP、DEEP_SLEEP_FALLBACK_S、low_power_deep_sleep_enter）。
    - `bmi160_any_motion_configure()` 阈值/持续时间寄存器交换修复。
    - `sensor_hub_sleep()` 所有 I2C 步骤增加错误检查和日志。
    - `esp_sleep_get_wakeup_causes()` 位掩码替代已弃用的 `esp_sleep_get_wakeup_cause()`。
### 2026-06-02 Light Sleep 文档实现 + WiFi 配网

40. **按 ESP-IDF 文档实现 Light Sleep**:
    - `low_power_sleep_enter()`: 每次调用前执行 `esp_sleep_enable_gpio_wakeup()` + `esp_sleep_enable_timer_wakeup(1s)`，然后 `esp_light_sleep_start()`。符合文档要求的 gpio_wakeup_enable → sleep_enable_gpio → light_sleep_start 三步流程。
    - LOW_POWER 两阶段：
      - **DEEP_WAIT**: 真正硬件 light sleep，CPU 断电。BMI160 INT1 触发 → GPIO 唤醒 → `esp_sleep_get_wakeup_causes()` 检测 → 确认运动 #1。
      - **MODEM_WAIT**: CPU 运行，`xSemaphoreTake(1s)` 等待。≥ 2s 后第二次运动 → 退出 LOW_POWER。若无运动 → 回退 DEEP_WAIT。

41. **新增 WiFi 配网组件 `wifi_cfg`**:
    - SoftAP SSID: `4G-Tracker`，密码 `12345678`。
    - HTTP 服务器端口 80，访问 `http://192.168.4.1` 进行配网。
    - 配置项：WiFi SSID/密码（可选）、服务器 IP/端口。
    - 提交后回调通知主程序，TCP 连接自动使用配置的服务器地址。
### 2026-06-02 修复：切回 PM 自动 sleep + NVS 初始化

42. **`esp_light_sleep_start()` 在 v6.0 上再次确认无法返回**，切回 PM + tickless-idle 自动 sleep：
    - DEEP_WAIT 阶段改用 `xSemaphoreTake(g_motion_sem, 1s)` + PM 自动 light sleep。
    - BMI160 ISR → 信号量 → CPU 唤醒 → phase 切换到 MODEM_WAIT。
43. **WiFi NVS 缺失修复**：`app_main()` 开头添加 `nvs_flash_init()`（含 erase 兜底），解决 `wifi osi_nvs_open fail ret=4353`。
### 2026-06-02 `config.h` 统一配置文件

44. **`pin_config.h` → `config.h`**，集中所有可配置参数：
    - 硬件引脚（UART/I2C/SPI/BMI160/ST7789/W25Q64）
    - 时序参数（GPTIMER 分辨率、低功耗超时、防抖间隔）
    - BMI160 运动阈值
    - WiFi AP SSID/密码/最大连接数
    - 服务器 IP/端口（同时用于 4G TCP 和 WiFi 配网）
    - 保留旧名称作为向后兼容别名，无需修改已有代码
    - `storage_mgr.c` 改用 `CFG_SERVER_PORT_DEFAULT`（snprintf 格式化）
### 2026-06-02 WiFi STA 连接 + 连通性测试 + Flash 存储

45. **WiFi 配网增强**:
    - 用户提交 SSID/密码后，ESP32 自动切换到 AP+STA 模式连接目标 WiFi。
    - 连接成功后通过 `getaddrinfo` + TCP connect 到 `CFG_PING_TARGET`（默认 `baidu.com:80`）验证外网连通性。
    - 结果页面实时显示：WiFi 连接状态 ✅/❌、连通性测试 ✅/❌。
    - `storage_config_t` 新增 `wifi_ssid[33]` 和 `wifi_password[65]` 字段，配网信息持久化存入 SPI Flash。
    - `config.h` 新增 `CFG_PING_TARGET`、`CFG_PING_PORT`、`CFG_WIFI_STA_CONNECT_TIMEOUT_MS`。
### 2026-06-02 `ct511n_4g_net_close()` 函数 + 编译依赖修复

46. **新增 `ct511n_4g_net_close()`**:
    - CT511N 模块新增关闭 4G 数据网络的完整函数，分三步：
      1. 退出透传模式（`+++`）
      2. 关闭 TCP 连接（`AT+CIPCLOSE=1`）
      3. 关闭数据网络 PDP 上下文（`AT+NETCLOSE`）
    - 该函数用于 WiFi 连接成功后关闭 4G 数据，避免蜂窝网络占用和功耗浪费。
    - 声明在 `CT511N.h`，实现在 `CT511N.c`。

47. **`wifi_cfg` 组件编译依赖修复**:
    - `wifi_cfg.c` 包含 `config.h` 间接引用 `driver/gpio.h`、`driver/spi_master.h`、`driver/uart.h`，以及 `storage_mgr.h` 间接引用 `W25Q64.h`。
    - `wifi_cfg/CMakeLists.txt` 的 `REQUIRES` 补充：`esp_driver_gpio`、`esp_driver_spi`、`esp_driver_uart`、`W25Q64`。
### 2026-06-02 WiFi 优先发送 + 4G 自动禁用

46. **WiFi 连接后优先通过 WiFi 发送 payload**:
    - `upload_send_all()` 开头检查 `wifi_cfg_is_sta_connected()`。
    - WiFi 在线 → 禁用 4G（`ct511n_sleep_dtr_enable`），用 `wifi_cfg_tcp_send()` 通过 lwip socket 直连服务器发送。
    - WiFi 离线 → 回退 4G CT511N 发送（原有逻辑）。
    - `wifi_cfg` 新增 `wifi_cfg_is_sta_connected()` 和 `wifi_cfg_tcp_send()` API。
### 2026-06-02 SPI Flash 适配 + 统一配置 + WiFi 自动连接

47. **W25Q64 驱动程序兼容 Micron Flash (JEDEC 0x20 70 17)**:
    - 放宽制造商 ID 校验：接受 Winbond (0xEF) 和 Micron (0x20)。
    - SPI 引脚调整：SCK=3, MOSI=4, MISO=2, CS=1（ESP32-C3）。
    - SPI Mode 0→3，增加 CS 预初始化（pull-up + 输出高电平），`gpio_reset_pin()` 释放 JTAG 占用。
    - 关闭 `CONFIG_ESP_DEBUG_OCDAWARE` 和 `CONFIG_ESP32C3_DEBUG_OCDAWARE`，释放 GPIO 2/3/4。
    - 时钟降至 10 MHz（面包板稳定）。

48. **服务器配置统一到 SPI Flash**:
    - 移除 `wifi_cfg` 中的 `g_server_ip`/`g_server_port` 局部变量。
    - WiFi 配网通过 `server_save()` 写入 SPI Flash，4G 通过 `storage_config_read()` 读取同一份配置。
    - 优先级：SPI Flash 存储值 → `config.h` 默认值。
    - `upload_send_all()` WiFi 路径使用 `wifi_cfg_get_server_ip/port()`（即 flash 值），4G 路径读 `storage_config_read()`。

49. **WiFi 开机自动连接**:
    - `wifi_cfg_start()` 初始化后从 SPI Flash 读取已存储的 SSID/密码，有则自动 STA 连接。

50. **修复 `sleep: Incorrect wakeup source` 错误**:
    - `low_power_sleep_unconfigure()` 移除不再需要的 `esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO)`（因已不用 `esp_light_sleep_start()`）。

### 2026-06-02 代码精简重构

51. **删除 `main/pin_config.h`**:
    - 该文件已完全冗余，所有引脚定义已在 `config.h` 中以 `CFG_` 前缀统一管理。

52. **`config.h` 去掉向后兼容别名**:
    - 移除 22 行旧别名宏（`CT_UART_PORT` → `CFG_CT_UART_PORT` 等）。
    - 所有源文件（`main.c`、`app_utils.c`、`app_utils.h`）直接使用 `CFG_` 前缀宏。
    - 移除不必要的 `#include "driver/gpio.h"`。
    - 新增 SPI 频率常量 `CFG_W25Q64_SPI_FREQ_HZ` / `CFG_ST7789_SPI_FREQ_HZ`。

53. **删除死代码**:
    - 移除 `low_power_sleep_enter()` 函数（声明 + 实现），该函数从未被调用。

54. **头文件清理**:
    - `app_utils.h`: 移除 `esp_sleep.h`、`ST7789.h`、`ble_img_rx.h`（仅在 .c 中使用）。
    - `app_utils.c`: 移除 `esp_sleep.h`，补充 `ST7789.h`、`ble_img_rx.h`。
    - `main.c`: 移除 `driver/uart.h`（已在 `config.h` 中包含），补充 `esp_sleep.h`、`ble_img_rx.h`。

55. **代码格式整理**:
    - 统一换行符为 CRLF，去除冗余空行。
    - 将所有 Unicode 特殊字符（em dash `—`）替换为 ASCII ` -- `，避免编码问题。
### 2026-06-02 SPI Flash 写入超时修复

51. **`storage_config_write()` 返回 `ESP_ERR_TIMEOUT (263)`**:
    - **根因**: Micron Flash 扇区擦除时间超过 `W25Q64_DEF_TIMEOUT_MS`（原 2s），`w25q64_wait_busy()` 在擦除完成前超时返回。
    - **修复**: 超时从 2s → 5s；等待循环中 SPI 读状态寄存器失败时不再立即返回错误，改为 delay 10ms 后重试。避免 Flash 忙时不响应 SPI 命令导致假超时。
    - **验证**: `server_save: x.x.x.x:port → flash (err=0)` 确认写入成功。4G TCP 连接使用配网设置的 IP/端口。
### 2026-06-02 config.h 补充采样间隔和 SPI 频率

52. **`config.h` 新增**:
    - `CFG_SAMPLE_INTERVAL_S` (1s)：传感器采样周期，替换 `main.c` 中硬编码。
    - `CFG_W25Q64_SPI_FREQ_HZ` (10 MHz)：SPI Flash 时钟频率，统一到配置文件。
### 2026-06-02 采样时间戳改用真实时间（GPS → WiFi SNTP → 4G CCLK → uptime）

53. **`storage_record_t.timestamp` 从 uptime 秒改为 Unix 时间戳**:
    - 新增 `get_unix_timestamp()` 函数，按优先级获取真实时间：
      1. GPS 卫星时间（`ct511n_gps_get_time()` 解析 GPSST 的 date+time 字段）
      2. WiFi SNTP（`time()` 返回已同步时间，WiFi STA 连上时自动启动 SNTP）
      3. 4G CCLK（`ct511n_4g_clk_get()` 解析模块时间）
      4. 回退 uptime 秒
    - GPS 未定位时跳过（date/time 字段为 0），WiFi 未连/未同步时跳过，4G 未就绪时跳过。
    - `ct511n_gps_get_time()` 新增函数：从 GPSST 响应中提取 `DDMMYY HHMMSS`。
    - WiFi 连上后自动启动 SNTP（`ntp.aliyun.com` + `pool.ntp.org`）。
### 2026-06-04 TCP 重连逻辑优化

56. **4G TCP 上传不再每次采样都重连**:
    - 新增 `s_server_connected` / `s_first_connect_done` 状态标志（`upload_send_all()` 内 static 变量）。
    - 首次采样: 最多 5 次 `ct511n_tcp_single_connect()` 尝试；全部失败则跳过上传，数据留在 Flash。
    - 后续采样: 仅 1 次重连尝试（不再浪费 15s+ 逐步骤重试）。
    - 已连接时直接 `ct511n_4g_tcp_send()`，不重新 connect。
    - 发送失败 → `s_server_connected = false`，停止当前批次上传。
    - `ct511n_tcp_single_connect()` 内部 CIPOPEN 重试 10→3。

57. **修复 LOW_POWER 退出后定时器间隔被重置为 1 秒**:
    - `low_power_exit()` 中 `timer_start(timer, 1)` 硬编码 → `timer_start(timer, CFG_SAMPLE_INTERVAL_S)`。
    - 根因: 每次退出低功耗后采样间隔从配置的 3s 被覆盖为 1s。

### 2026-06-04 QYEG0397 EPD 墨水屏驱动

58. **新增 `components/epd_qyeg0397/` 驱动组件**:
    - 控制器: Solomon SSD1680 兼容，波形存储在 OTP，4 色 (黑/白/红/黄)。
    - 分辨率: 800×480，2-bit 打包格式 (4 像素/字节，96000 字节/帧)。
    - SPI 4-wire Mode 0，共享 SPI2_HOST 与 W25Q64/ST7789 分时复用。
    - 引脚: CS=15, DC=16, RST=17, BUSY=18（`config.h` 中 `CFG_EPD_*`）。

59. **EPD 字体渲染模块 `epd_font.h/c`**:
    - 8×16 等宽位图字体，覆盖 ASCII 32-126 (95 字符)。
    - `epd_draw_char/string/string_wrap()` — 像素级文本绘制。
    - 字体位图 1520 字节。

60. **EPD 传感器叠加 `epd_sensor_overlay()`**:
    - 墨水屏右下角 264×160 白色面板 + 黑色边框，实时显示：
      ACC:  0.12 -0.34  1.05 g
      GYR:  0.0   0.0   0.0 dps
      MAG: 25.3  38.1 -12.7 µT
      GPS: 39.9042,116.4074

    - 使用部分刷新 (`epd_update_partial()`)，每次 ~4-6 秒（比全刷 18 秒快）。
    - 每采样后自动更新。

61. **BLE 图像 → EPD 显示**:
    - `epd_display_from_flash()` — 从 W25Q64 IMG_CACHE 区读图像 → EPD 全屏刷新。
    - `epd_ble_img_ready()` — BLE 接收完成回调，显示到墨水屏。
    - `STATE_IMG_RECEIVE` 改为启动 BLE + EPD 回调（不再依赖 ST7789 LCD）。
    - 图像格式: 原始 96000 字节 2-bit EPD 数据（手机端直接发送）。

62. **共享 SPI 总线架构**:
    - `epd_init_shared_bus()` — EPD 挂载到 W25Q64 已初始化的 SPI2_HOST。
    - EPD 申请独立 CS/DC/RST/BUSY GPIO，SPI Mode 0，不冲突。

63. **SPI DMA 开启**: EPD 传输 96KB 数据必须用 DMA (`SPI_DMA_CH_AUTO`)，旧代码写死 `SPI_DMA_DISABLED`。

64. **按规格书修正 EPD 寄存器**:
    - BUSY 极性: 规格书明确定义 Low=忙, High=闲，代码原判断 `== 1` 反了。
    - CDI (0x50): 0x97 → 0x37 (OTP 参考代码)。
    - BTST (0x06): 0xA1 → 0xA4 (OTP 参考代码)。
    - DRF (0x12): 需附带 0x00 数据字节。
    - 新增完整 "Enter Solomon Command" 初始化序列: PSR、PLL、GSST、TEMP、扩展寄存器 (0xE0/0xE7/0xE9)。

65. **组件编译配置**: `epd_qyeg0397/CMakeLists.txt` 新建，`main/CMakeLists.txt` 新增依赖。

### 2026-06-04 采样后关闭磁力计 + 陀螺仪

66. **采样间传感器省电**:
    - 新增 `sensors_power_save()` / `sensors_power_restore()`（`app_utils.c`）。
    - 每次 `sample_sensors()` 结束后:
      - AK09911C → `MODE_POWERDOWN` (~0.4µA，省 ~1mA)
      - BMI160 陀螺仪 → `MODE_SUSPEND` (~50µA，省 ~850µA)
    - 加速度计保持 NORMAL 模式（any-motion 唤醒需要）。
    - 下次采样前自动恢复：陀螺 → NORMAL，磁力计 → CONT_10HZ。
    - 磁力计首读数可能未就绪 (~100ms 启动时间)，`ak09911_data_ready()` 自动跳过。

### 2026-06-04 项目文件结构更新

```raw
84. **BLE 改为开机即广播**:
    - **变更**: 移除了 `STATE_IMG_RECEIVE` 中的 `ble_img_init()` / `ble_img_deinit()` 调用。
    - `app_main()` 中 WiFi AP 启动后调用 `ble_img_init(epd_ble_img_ready)` 一次性初始化 BLE。
    - BLE 广播持续运行，设备名 `"4G-Tracker"`。
    - `STATE_IMG_RECEIVE` 仅设置 RAM 缓存模式、等待传输完成 (`!ble_img_is_busy()`)、刷新缓存后退出。
    - 传输完成后 `epd_ble_img_ready()` 回调直接调用 `epd_update_full_from_flash()` 显示图像。

### 2026-06-05 BLE 电源优化 + WiFi 自动管理

85. **BLE init 移在 WiFi 之前（防崩溃）**:
    - **根因**: BLE 控制器初始化需大量连续内存，WiFi AP 启动后堆碎片化导致 `Malloc failed` → `ble_gatts_count_cfg` 空指针崩溃。
    - **修复**: `app_main()` 中 `ble_img_init()` 移到 `wifi_cfg_start()` 之前调用。

86. **BLE 电源优化 — 空闲停止广播，运动恢复**:
    - `low_power_enter()` 中调用 `ble_adv_stop()` 停止广播。
    - `low_power_exit()` 中调用 `ble_adv_start()` 恢复广播。
    - 广播间隔从默认 30ms 改为 1~2 秒 (`ble_adv_start()` 自定义 `adv_params`)。
    - `ble_img_rx.c` 新增 `ble_adv_start()` / `ble_adv_stop()` API。
    - `ble_img_rx.h` 重写，清理新旧内容重叠导致的重复定义。

87. **WiFi 自动管理 — 无 STA 则关闭，10s 扫描，双击配网**:
    - `app_main()` 中启动 WiFi AP 后等待 8 秒，若无可连接 STA 则调用 `wifi_cfg_stop()` 关闭 AP。
    - `STATE_ACTIVE` 循环中：AP 关闭时每 10 秒尝试 `wifi_cfg_start()` 重连。
    - 双击 BMI160：AP 关闭时 → 启动 AP 配网模式 20 秒；AP 开启时 → 进入 IMG_RECEIVE。

88. **`wifi_cfg_start()` 支持重复调用**:
    - **根因**: `wifi_cfg_start()` 每次调用都执行 `esp_netif_init()` / `esp_event_loop_create_default()` / `esp_netif_create_default_wifi_ap()`，第二次调用时因重复创建 netif 导致 `assert failed: esp_netif_new_api (duplicate key)`。
    - **修复**: `wifi_cfg.c` 重构，分离 one-time init 和 WiFi start：
      - `esp_netif_init()` / `esp_event_loop_create_default()` / `esp_wifi_init()` 仅首次执行（`g_wifi_inited` 标志）。
      - `esp_netif_create_default_wifi_ap()` 只创建一次（`g_ap_netif` 指针）。
      - `wifi_cfg_stop()` 不再调用 `esp_wifi_deinit()`，保留初始化状态供重复 start。
### 2026-06-05 WiFi 冲突修复 — 缓存 + 纯动态 RX + BLE 优先

89. **WiFi 第二启动 SPI DMA 崩溃 — 缓存 config 避免 SPI 读取**:
    - `wifi_cfg.c` 新增 `wifi_auto_connect()` 函数，首次调用时缓存 `storage_config_read()` 结果，后续复用缓存避免 SPI Flash DMA 分配失败。
    - 移除 `goto sta_connect` 标签和重复的 `storage_config_read` 代码。

90. **WiFi task 创建失败 + BLE advertising 被抢占 — 全面内存优化**:
    - **根因**: BLE NimBLE 初始化后堆碎片化，WiFi 驱动的静态 RX 缓冲区 (1600 字节连续) 无法分配 → WiFi task 创建失败。
    - `sdkconfig` / `sdkconfig.defaults`:
      - 禁用 NimBLE 未使用角色 (CENTRAL/OBSERVER/GATT_CLIENT/SECURITY)
      - 减少 NimBLE 内存池 (MSYS_1=6, MSYS_2=12, ACL=12, MTU=128)
      - 减少 WiFi RX/TX 缓冲区 (STATIC_RX=1, DYNAMIC_RX=8, DYNAMIC_TX=16)
      - 禁用 AMPDU、IRAM_OPT、SAE 支持
    - `wifi_cfg.c`: 检查 `esp_wifi_init()` 返回值，失败返回 `ESP_ERR_NO_MEM`。
    - `ble_img_rx.c`: `advertising complete` 日志增加 `reason` 码。

91. **`storage_config` 读取缓存 — 避免每次 SAMPLE 读 SPI Flash**:
    - `app_utils.c` `upload_send_all()` 改用 `s_cached_cfg` 静态缓存，SPI Flash 仅读一次。
    - 避免 SPI DMA 分配失败导致 Guru Meditation。

92. **BLE 广告被 WiFi PHY 抢占后自动重启**:
    - `main.c`: `wifi_cfg_start()` 失败后调用 `ble_adv_start()` 恢复广播。
    - 10s 扫描循环添加重试上限 (5次)，超过后等待 2 分钟再试。

93. **WiFi 静态 RX 缓冲区改为纯动态 (static_rx_buf_num=0)**:
    - `wifi_cfg.c`: 运行时覆盖 `wifi_init_config_t`，设 `static_rx_buf_num=0`，避免碎片化堆中分配 1600 字节连续块。
    - BLE 先初始化 (保证 HCI 正常)，WiFi 后初始化 (纯动态 RX 无大块需求)。

94. **BLE 初始化次序修复 — WiFi 部分初始化破坏 HCI**:
    - **根因**: WiFi init 先执行会破坏共享无线电控制器，导致 NimBLE VHCI (`hci inits failed`) 崩溃。
    - **修复**: 恢复 BLE → WiFi 顺序，WiFi 使用纯动态 RX 适配碎片化堆。


95. **`static_rx_buf_num` 有效范围修正 — Kconfig 最小值是 2**:
    - **根因**: Kconfig 定义 `range 2 25`（ESP32-C3 无 HE 支持），0 和 1 均被 WiFi 驱动拒绝 (`out of range`)。
    - **修复**: 设为最小值 2（需 2×1600 = 3200 字节 DMA 内存）。首次 init 在 BLE 之后但堆尚有大块时通常能成功分配。

```

96. **WiFi DMA 内存预分配 — BLE 前预留、WiFi 前释放**:
    - **根因**: BLE NimBLE 初始化后堆碎片化，WiFi 需要 2×1600 字节 DMA 独立块但只能找到 1 块。
    - **修复**: `main.c` 在 BLE init 前用 `heap_caps_malloc(MALLOC_CAP_DMA, 3200)` 预分配 3200 字节 DMA 内存。BLE init 后、WiFi init 前 `free()` 释放，为 WiFi 创建新鲜连续的 DMA 内存洞。


97. **WiFi DMA 专属池 — 通过 `wifi_osi_funcs` 劫持 `_malloc/_free`**:
    - **根因**: `free()` 释放的孔洞被 BLE 重启等异步操作占用，WiFi 仍拿不到。
    - **修复**: `wifi_cfg.c` 预分配 2×1600B DMA 缓冲区（`wifi_cfg_reserve_dma()`，BLE 前调用），克隆 `g_wifi_osi_funcs` 替换 `_malloc`/`_free`/`_malloc_internal`/`_wifi_malloc` 为池分配器。池中缓冲区**永不释放**，WiFi 驱动内部分配时直接从池返回。
    - 所有未覆盖的函数指针保持默认值，**严禁设为 NULL**（`_zalloc_internal=NULL` 曾导致空指针崩溃）。

98. **ESF 缓冲区分配失败 (`esf_buf_setup_static: alloc eb fail(10)`)**:
    - **根因**: 池大小为 2×1600B 仅覆盖静态 RX 缓冲区。WiFi 驱动还需要 ESF (Enhanced Stream Framework) 缓冲区，由 `_malloc_internal`/`_wifi_malloc` 分配，这些函数也已被劫持但池已耗尽或请求大小 > 1600。
    - **状态**: 待修复 — 可能需要增大池容量。


99. **ESF 缓冲区崩溃修复 — 按大小分流，大小 != 1600 回退系统堆**:
    - **根因**: 池分配器用 `size <= 1600` 判断，ESF 控制块等小分配也走池，池耗尽后返回 NULL → WiFi ROM `ppRxFragmentProc` 空指针解引用崩溃。
    - **修复**: `wifi_pool_malloc()` 等函数改为 `size == 1600` 才从池分配，其余大小（ESF 控制块、管理帧等）直接回退 `s_orig_malloc`（系统堆）。`_malloc_internal` / `_wifi_malloc` 各自有独立的回退保存 (`s_orig_malloc_internal` / `s_orig_wifi_malloc`)。
    - 小分配对碎片化不敏感（系统堆始终能提供几十字节的内存）。


100. **ESF 分配崩溃彻底修复 — 纯正原生回退（Native Pass-through）**:
    - **根因**: 池分配器用 `size == 1600` 精确匹配，但 ESF 等小分配走 `pool_try_alloc` 返回 NULL 后虽然会回退到 `s_orig_*`，但之前版本 `_malloc_internal` 和 `_wifi_malloc` 共用同一个 `s_orig_malloc`（而非各自独立的原生指针），导致回退时内存属性不匹配。
    - **修复**: 
      - `_malloc` / `_malloc_internal` / `_wifi_malloc` 各自保存独立的原生指针（`s_orig_malloc` / `s_orig_malloc_internal` / `s_orig_wifi_malloc`），回退时精确传递到正确的原生函数。
      - Size 判断改为 `[1600, 1700]` 范围匹配（容应对齐 Padding）。
      - Free 函数改用地址范围判断（`s_pool_start` ~ `s_pool_end`），无需遍历。
      - `pool_try_alloc()` 提取为 `inline` 公共函数，减少代码重复。
    - 用户称赞了日志 #92 的优雅错误恢复（`WiFi init failed — restarting BLE advertising`）。


101. **终极瘦身 — 压缩 WiFi 默认配置**:
    - `wifi_cfg.c` 中 `esp_wifi_init()` 前强制覆盖:
      - `dynamic_rx_buf_num = 2` (默认 ~8)
      - `dynamic_tx_buf_num = 4` (默认 16)
      - `ampdu_rx_enable = 0`
      - `ampdu_tx_enable = 0`
    - 关闭 AMPDU 节省大量 DMA 内存。对于纯文本级别上传应用完全足够。


102. **终极杀器 — 释放经典蓝牙内存 + 极限压缩 + 诊断日志**:
    - **`main.c`**: NVS 初始化后调用 `esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)`，释放经典蓝牙占用的 ~30KB DMA 内存（我们用 NimBLE, BLE only）。
    - **`wifi_cfg.c`**: 
      - `dynamic_tx_buf_num = 2` (绝对下限)
      - `rx_mgmt_buf_num = 2` (默认 5)
      - 添加 `--- Before WiFi Init ---` 诊断日志：Free 8BIT/DMA Heap、Largest Free DMA Block


### 2026-06-05 BLE/WiFi 状态机重构 — 错峰启动 + 优雅降级

103. **全新上电顺序：WiFi STA-only → BLE**:
    - 去掉了老旧的"BLE→WiFi"顺序和 10s 扫描。
    - 上电: EPD → **WiFi STA-only (无 BLE)** → 5s 尝试连接已保存网络 → 关 WiFi → **BLE 初始化**。
    - 利用开机最干净的堆给 WiFi 联网；WiFi 关闭后 BLE 拥有完整内存。

104. **双击 BMI160 → 暂停 BLE + AP 配网 60s**:
    - `ble_img_pause()` 彻底释放 NimBLE (~50KB)，腾空内存给 WiFi AP。
    - AP 模式 20s 等待设备连接，已连接后延长到 60s 等待保存配置。
    - 保存配置或超时后 `ble_img_resume()` 恢复 BLE。
    - 新增 `wifi_cfg_start_ap_only()` / `wifi_cfg_start_sta_only()` 分离 AP 和 STA 行为。

105. **周期扫描：5 分钟一次替代 10 秒无脑轮询**:
    - 每 5 分钟: 停 BLE 广播 → WiFi STA 快速扫描 → 恢复 BLE。
    - 功耗降低 ~80%（10 秒一次 vs 5 分钟一次）。

106. **LOW_POWER 保持原样**:
    - 进入 `ble_adv_stop()` + 传感器休眠；退出 `ble_adv_start()`。

107. **新增 API**:
    - `wifi_cfg_start_sta_only()` — 仅 STA 模式
    - `wifi_cfg_start_ap_only()` — 仅 AP 模式（配网用）
    - `wifi_cfg_is_ap_sta_connected()` — AP 是否有设备连接
    - `ble_img_pause()` / `ble_img_resume()` — 暂停/恢复 BLE（释放 NimBLE 内存）


108. **配置参数集中化 + WiFi 上传速率限制**:
    - `config.h` 新增 `CFG_WIFI_UPLOAD_INTERVAL_US` (5min)
    - `app_utils.c` `upload_send_all()` WiFi 路径添加静态速率限制：两次 WiFi 上传间隔 ≥ `CFG_WIFI_UPLOAD_INTERVAL_US`。采样数据持续存入 Flash，每 5 分钟批量上传一次。
    - 4G 模式保持原有行为：每次采样后尝试上传。


109. **BLE 控制器 32KB EM 崩溃 — 开机预初始化**:
    - **根因**: WiFi STA 先初始化碎片化堆 → `ble_img_init()` → `nimble_port_init()` → `esp_bt_controller_init()` 找不到 32KB 连续 EM (Exchange Memory) → assert `emi.c:164` 崩溃。
    - `ble_img_rx.h/c`: 新增 `ble_img_ctrl_init()` 在开机堆最干净时预初始化 BT 控制器（分配 32KB EM）。`ble_img_init()` 检查 `g_bt_ctrl_inited` 标志，若控制器已初始化则跳过 `esp_bt_controller_init()`，只执行 `esp_bt_controller_enable()` + `esp_nimble_init()`。
    - `main.c`: NVS 初始化后立即调用 `ble_img_ctrl_init()`，EDP/WiFi/BLE 顺序前先占住 32KB 连续内存。
    - `config.h`: `CFG_WIFI_UPLOAD_INTERVAL_US` (5min) WiFi 批量上传限流。


110. **Host 初始化失败 + 空指针崩溃 — 双杀修复**:
    - **根因**: WiFi 仅 `esp_wifi_stop()` 未 `esp_wifi_deinit()`，驱动内存 (30KB+) 仍占用 → NimBLE Host `esp_nimble_init()` 失败 → 仍创建 host task → 空事件队列 null pointer crash。
    - `ble_img_rx.c`:
      - `ble_img_init()`: 检查 `nimble_port_init()` / `esp_nimble_init()` 返回值，失败直接返回，不创建 host task。新增 `g_ble_init_ok` 标志。
      - `ble_img_pause()`: 完全释放 BLE — `nimble_port_stop()` + `nimble_port_deinit()` + `esp_bt_controller_disable()` + `esp_bt_controller_deinit()`。
      - `ble_img_resume()`: 重新 `ble_img_ctrl_init()` + `ble_img_init()`。
      - `ble_img_ctrl_init()` 返回类型从 `void` 改为 `esp_err_t`。
    - `wifi_cfg.h/c`: 新增 `wifi_cfg_deinit()`，调用 `esp_wifi_deinit()` + `esp_netif_deinit()`，完全释放 WiFi 驱动内存 (~30KB)。
    - `main.c`:
      - 开机 STA 失败后 `wifi_cfg_deinit()` 替代 `wifi_cfg_stop()`。
      - 双击 AP 模式：`ble_img_pause()` → `wifi_cfg_reserve_dma()` + `wifi_cfg_start_ap_only()` → `wifi_cfg_deinit()` → `ble_img_resume()`。
      - 周期扫描：`wifi_cfg_reserve_dma()` + `wifi_cfg_start_sta_only()` → `wifi_cfg_deinit()`。


111. **周期扫描逻辑重写 — 严格时分复用**:
    - 上次 WiFi 连接**成功** → 等 `CFG_WIFI_UPLOAD_INTERVAL_US` (5min) 后重连上传
    - 上次 WiFi 连接**失败** → 等 `CFG_PERIODIC_SCAN_INTERVAL_US` (5min) 后重扫
    - 每次 WiFi 前后：`ble_img_pause()` 完全释放 BLE 内存 → WiFi 操作 → `ble_img_resume()` 重启 BLE
    - 修复了多余的 `}` 导致的 switch 结构断裂编译错误。


112. **WiFi 重复 deinit 警告 0x3001 修复**:
    - `wifi_cfg_deinit()` 添加 `g_wifi_inited` 守卫，已 deinit 时直接跳过。
    - 事件处理器实例改为静态变量 `g_inst_wifi`/`g_inst_ip`，deinit 时主动 `unregister` 避免回调残留触发二次释放。

113. **周期扫描严格时分复用 + 状态跟踪**:
    - 上次 WiFi 连接成功 → `CFG_WIFI_UPLOAD_INTERVAL_US` 后重连上传
    - 上次 WiFi 连接失败 → `CFG_PERIODIC_SCAN_INTERVAL_US` 后重扫
    - 每次操作前后：`ble_img_pause()` 完全释放 BLE → WiFi → `ble_img_resume()` 重启 BLE

114. **清理死参数**:
    - 删除 `CFG_BOOT_STA_TIMEOUT_US`（从未被任何 .c 引用，实际超时由 `CFG_WIFI_STA_CONNECT_TIMEOUT_MS` 控制）


115. **BLE 广播间隔改为 config.h 可调**:
    - `config.h` 新增 `CFG_BLE_ADV_ITVL_MIN` / `CFG_BLE_ADV_ITVL_MAX`，单位 0.625ms，默认 1600/3200 (1s/2s)。
    - `ble_img_rx.c` 改用这些常量替代硬编码。

116. **BT 控制器任务栈和优先级配置**:
    - `ble_img_ctrl_init()` 覆盖 `controller_task_stack_size = 4096`、`controller_task_prio = 5`。


### 2026-06-05 BLE 广告崩溃修复 — ESP_BT_MODE_BLE enum 陷阱

117. **`#define ESP_BT_MODE_BLE 0` fallback 覆盖了 enum 常量**:
    - **根因**: `esp_bt.h` 中 `ESP_BT_MODE_BLE` 是 **enum 常量** (`0x01`)，不是 `#define` 宏。代码中的 `#ifndef ESP_BT_MODE_BLE` 检查的是预处理宏定义，enum 常量不通过 → fallback `#define ESP_BT_MODE_BLE 0` 触发 → `esp_bt_controller_enable(0)` 传参为 `ESP_BT_MODE_IDLE` → 返回 `ESP_ERR_INVALID_ARG` ("invalid mode 0")。
    - **修复**: 移除整个 `#ifndef` fallback 块。直接 `#include "esp_bt.h"` 即可，无需任何兜底定义。

118. **`ble_img_ctrl_init()` 重复 enable 控制器**:
    - **根因**: 原 `ble_img_ctrl_init()` 仅调用 `esp_bt_controller_init()`，不 enable。enable 在 `ble_img_init()` 中完成。重构后 `ble_img_ctrl_init()` 同时做 init + enable，但 `ble_img_init()` 中 `g_bt_ctrl_inited` 分支仍尝试二次 enable → 失败。
    - **修复**: `ble_img_ctrl_init()` 增加 `esp_bt_controller_get_status()` 三态判断：
      - `ESP_BT_CONTROLLER_STATUS_ENABLED` → 跳过，直接返回 OK
      - `ESP_BT_CONTROLLER_STATUS_INITED` → 只需 enable
      - 其他 → init + enable
    - `ble_img_init()` 中 `g_bt_ctrl_inited` 分支移除 `esp_bt_controller_enable()`，仅调用 `esp_nimble_init()`。

119. **`ble_adv_stop()` 空指针崩溃 — 缺 `ble_hs_is_enabled()` 检查**:
    - **根因**: NimBLE Host 未启用时调用 `ble_gap_adv_stop()` → 内部访问 `ble_hs_is_enabled()` → host 模块未初始化 → 空指针 → Load access fault。
    - **修复**: `ble_adv_stop()` 先检查 `if (!ble_hs_is_enabled()) return;`。

### 2026-06-05 低功耗卡死修复 — DISCONNECT 回调 + 缺失的 `esp_sleep_enable_gpio_wakeup()`

120. **LOW_POWER 被 BLE DISCONNECT 回调重启广告卡死**:
    - **根因**: 进入 LOW_POWER 时 `ble_adv_stop()` 只停了广播。若此前有 BLE 客户端连接着，客户端的断连会触发 `BLE_GAP_EVENT_DISCONNECT` → 回调调用 `ble_gap_adv_start()` 重启广告 → 系统在 LOW_POWER 状态但广告在跑，永远等不到运动唤醒。
    - **修复**: 添加 `g_sleeping` 标志（`ble_adv_stop()` 置 true，`ble_adv_start()` 清 false）。DISCONNECT 回调检查 `g_sleeping`，为 true 时跳过广告重启。

121. **`esp_sleep_enable_gpio_wakeup()` 每次 sleep 后需重新使能**:
    - **根因**: `esp_sleep_enable_gpio_wakeup()` 在每次 light-sleep 后被硬件自动消费掉。PM 框架在 ESP-IDF v6.0 上不会自动重调。原代码只在一个入口调一次 → 后续所有 sleep 周期 GPIO wakeup 都失效。
    - **第一次修复**: `low_power_sleep_configure()` 增加 `esp_sleep_enable_gpio_wakeup()` 调用。
    - **第二次修复（真正生效）**: 在 `STATE_LOW_POWER` / `LP_DEEP_WAIT` 的每次 `xSemaphoreTake` 循环前重新调用 `esp_sleep_enable_gpio_wakeup()`，确保每个 sleep 周期都使能 GPIO 唤醒。

### 2026-06-05 `adv_start failed: 6` — 异步 stop 未完成

122. **唤醒后 `ble_adv_start()` 返回 6（BLE_HS_ENOMEM）**:
    - **根因**: `ble_adv_stop()` → `ble_gap_adv_stop()` 是异步操作，实际停止由 NimBLE host task 处理。调用后主 task 立即进入 light-sleep，host task 被冻结，停止指令未执行。唤醒后尝试 `ble_adv_start()` 时 host 仍在"停止中"状态 → 冲突。
    - **修复**: `low_power_enter()` 中 `ble_adv_stop()` 后添加 `vTaskDelay(150ms)`，让 host task 有时间完成停止流程再进入 sleep。

### 2026-06-05 BLE 广播间隔不生效 — `ble_adv_start()` 硬编码未用 config.h

123. **`ble_adv_start()` 忽略 `CFG_BLE_ADV_ITVL_MIN/MAX`**:
    - **根因**: `ble_adv_start()` 中广告间隔写死为 `1600`/`3200`（1s/2s），完全不读 `config.h` 的 `CFG_BLE_ADV_ITVL_MIN/MAX`。用户修改 config.h 但代码无效。
    - **修复**: 
      - 将硬编码替换为 `CFG_BLE_ADV_ITVL_MIN` / `CFG_BLE_ADV_ITVL_MAX`。
      - 在 `ble_img_rx.c` 添加 `#include "config.h"`（放在 fallback define 之前）。
      - 在 `ble_img_rx/CMakeLists.txt` 添加 `"main"` 到 `component_reqs`，使组件能访问 main 组件的 config.h。
      - 修正 `config.h` 中注释（5 × 0.625ms = 3.125ms，非 1 秒）。

### 2026-06-05 服务注册移至 init 阶段 + 同步回调精简

124. **特征值绑定句柄 + 服务注册提前到 `ble_img_init()`**:
    - 特征值定义添加 `.val_handle = &g_data_chr_val_handle` / `&g_ctrl_chr_val_handle`。
    - 将 `ble_gatts_count_cfg()` / `ble_gatts_add_svcs()` 从同步回调 (`ble_img_on_sync`) 移至 `ble_img_init()` 中 host init 后、host task 启动前执行。
    - 同步回调精简为仅设置广播字段和启动广告。
    - 修复 `adv_set_fields: 4` 错误（广播数据超 31 字节）。

### 2026-06-06 `ble_img_pause` 简化为仅停广告

125. **移除 `ble_img_pause()` / `ble_img_resume()` 中的完全 deinit/reinit**:
    - **根因**: 完全 deinit/reinit BLE（释放控制器+host 内存）在每次 WiFi 切换时引入 ~500ms 延迟，且 `esp_bt_controller_init()` 可能因内存碎片失败。
    - **修复**: `ble_img_pause()` 仅调用 `ble_adv_stop()`，`ble_img_resume()` 仅调用 `ble_adv_start()`。ESP32-C3 的 Modem Sleep 机制在广播停止后自动关闭 RF 硬件，功耗与完全 deinit 一致，但内存和 GATT 服务配置全部保留。

### 2026-06-06 图像传输防栈溢出 + 无响应写入

126. **`img_data_write()` 增加长度裁剪保护**:
    - **根因**: Chrome Web Bluetooth 可能协商大 MTU（~512B），若 `chunk_len > BLE_IMG_CHUNK_SIZE_MAX` (512) 则栈数组溢出，擦写 FreeRTOS 返回地址导致任务死锁。
    - **修复**: 增加 `if (chunk_len > BLE_IMG_CHUNK_SIZE_MAX) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;`。

127. **数据特征值改为无响应写入 (`BLE_GATT_CHR_F_WRITE_NO_RSP`)**:
    - **根因**: 图像传输使用带响应写入时，每包需等待 ESP32 写入 Flash 并回复，Flash 写入耗时导致网页 BLE 超时断开。
    - **修复**: 数据特征值改为 `BLE_GATT_CHR_F_WRITE_NO_RSP`；控制命令（START/DONE/CANCEL）保留 `BLE_GATT_CHR_F_WRITE` 确保可靠到达。
    - 网页端需对应使用 `writeValueWithoutResponse()`。

### 2026-06-06 状态机 BLE 传输保护重构

128. **STATE_ACTIVE 入口 BLE 忙检查**:
    - `STATE_ACTIVE` 循环入口增加 `ble_img_is_busy()` 检查，如果蓝牙正忙立即切到 `STATE_IMG_RECEIVE`，阻止任何 WiFi 扫描/4G 上传/低功耗入眠打断传输。

129. **STATE_SAMPLE 拆碎延时 + 三重 BLE 拦截**:
    - 将 `vTaskDelay(CFG_CT_WAKE_DELAY_MS)` 拆碎为 20ms 小片，每片之间检查 `ble_img_is_busy()`，避免长延时不可打断。
    - 采样前、延时中、采样后共三重 BLE 忙检查，采样期间 BLE 启动则跳过 4G 上传直接切 `IMG_RECEIVE`。
    - 防止 4G TCP 拨号阻塞期间 BLE 无响应。

130. **STATE_IMG_RECEIVE 阶梯超时机制**:
    - **已收字节 = 0**（手机还在准备/仿色）：35 秒宽容期超时。
    - **已收字节 > 0**（传输进行中）：6 秒断流超时。
    - 通过 `ble_img_bytes_received()` 动态监控进度，有新数据则刷新计时器。
    - 超时后强制清 RAM 缓存、回 `STATE_ACTIVE`，防止设备变砖。

131. **BLE 传输期间跳过采样**:
    - `STATE_IMG_RECEIVE` 中 `g_sample_flag` 只清标志不执行 `sample_sensors()`，避免 SPI Flash 竞争影响图像写入。

### 2026-06-06 `0xFA` 占线锁 + STATE_SAMPLE 延时拆碎

132. **`0xFA` 占线锁预锁 BLE 忙碌状态**:
    - **问题**: 网页端连接 BLE 后在准备阶段（仿色/编码）有 ~500ms 空白期，此时 4G 可能启动上传阻塞 CPU。
    - **修复**: `img_data_write()` 入口拦截单字节 `0xFA`，收到后设 `g_busy = true`、`g_received_bytes = 0`，不写入 Flash。网页端在传图前先发 `writeValueWithoutResponse([0xFA])` 预锁 BLE，`ble_img_is_busy()` 立即返回 true 熔断 4G。

133. **`STATE_SAMPLE` 延时拆碎为 20ms 片**:
    - 将 `vTaskDelay(CFG_CT_WAKE_DELAY_MS)`（~500ms）拆为 20ms×25 片，每片之间检查 `ble_img_is_busy()`，可在延时中途响应 BLE 占线锁。

### 2026-06-06 `img_cache_write` 跨页循环 + NimBLE mbuf 扩容 + `0xFA` 占线锁

134. **`img_cache_write` 跨页循环代替截断**:
    - **问题**: 原函数遇 W25Q64 256 字节页边界时直接截断丢数据 (`truncated` 日志)。
    - **修复**: `while(len>0)` 循环，每页写满后自动写入下一页，数据不丢失。

135. **NimBLE mbuf 池扩容**:
    - `MSYS_1_BLOCK_COUNT`: 6→16，`MSYS_2_BLOCK_COUNT`: 12→24，`ACL_FROM_LL_COUNT`: 12→24，防止 `ACL buf alloc failed`。

136. **`MTU` 提升至 256**: `CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU`: 128→256。

### 2026-06-06 EPD 刷屏解耦 + 主任务栈扩容 + 堆分配

137. **EPD 刷屏从 BLE 回调移至 main 任务**:
    - **问题**: `epd_ble_img_ready()` 在 NimBLE host task 上下文中直接调 `epd_update_full_from_flash()`（~18s），阻塞蓝牙协议栈。
    - **修复**: `epd_ble_img_ready()` 仅设 `g_need_epd_update = true` 标志，main 循环每次迭代检查标志后在 main task 上下文中安全执行刷屏。

138. **主任务栈扩容**: `CONFIG_ESP_MAIN_TASK_STACK_SIZE`: 3584→8192（8KB）。

139. **EPD 4KB 缓冲改用堆分配**: `uint8_t chunk[4096]`（栈）→ `malloc(4096)`（堆），避免栈溢出。

140. **`img_cache_write` 挂起 NimBLE host task**:
    - 在写 Flash 前 `vTaskSuspend(ble_host_task)`，写完后 `vTaskResume`，防止写 Flash 期间 MBUF 耗尽崩溃。
    - 移除传输中动态 `img_cache_erase_sector` 调用，整片擦除在 `IMG_CMD_START` 时一次性完成。

### 2026-06-06 SPI DMA 静态化 + 内存极限瘦身 + 启动顺序调整

141. **SPI DMA bounce buffer 改用 `heap_caps_malloc(MALLOC_CAP_DMA)`**:
    - `w25q64_spi_txrx()` 中 `malloc` → `heap_caps_malloc(..., MALLOC_CAP_DMA)`，确保 DMA 内存可用。

142. **全局静态 DMA 缓冲区 `s_spi_dma_buf[512]`**:
    - 系统启动时预占 BSS 段连续 DMA 内存，永不释放。
    - `w25q64_spi_txrx()` 双路径：≤512B 用静态 buf（零动态分配），＞512B fallback 动态。
    - 加互斥锁 `s_spi_dma_lock` 保护，支持多任务并发访问 SPI。

143. **sdkconfig.defaults 极限瘦身**:
    - WiFi: `STATIC_RX_BUF=4`, `DYNAMIC_RX_BUF=2`, `DYNAMIC_TX_BUF=2`, `TX_BA_WIN=2`, `MGMT_SBUF=4`, AMPDU 全部关闭。
    - NimBLE: `MSYS_1=8`, `MSYS_2=16`, `ACL=16`（此前从 6→16 提得太高，回退）。
    - FreeRTOS timer stack: 2048。
    - Log level: `WARN`（日志字符串省 RAM）。

144. **启动顺序调整：BLE 优先，WiFi 最后**:
    - 旧: `hardware_init → EPD → WiFi STA → deinit → BLE init`
    - 新: `hardware_init → EPD → BLE init → WiFi STA → deinit`
    - BLE 先初始化（堆最干净），WiFi 放到最后（用剩余堆）。
    - WiFi 初始化失败不影响 BLE 正常工作。`


### 2026-06-06 内存日志增强 + 模块 deinit 显示清理

145. **所有模块 deinit 后增加内存释放日志**:
    - `wifi_cfg_deinit()`: 末尾打印 `MEM after deinit: Free/MaxBlock/DMA_MaxBlock`。
    - `hardware_init()` 错误路径: `ct511n_destroy()` 后调 `LOG_MEM_AFTER`。
    - `epd_deinit()` / `wifi_cfg_deinit` 各处均已覆盖 `LOG_MEM_AFTER`。

146. **`wifi_cfg_start_sta_only()` 重写为健壮模式**:
    - 每次调用执行完整 stop→deinit→reinit 周期（替代原来的一次 init、重复使用的设计）。
    - 去掉 `esp_wifi_restore()` 和 `vTaskDelay(1000)` 临时 workaround。

### 2026-06-06 sdkconfig 调试增强

147. **`CONFIG_HEAP_POISONING_COMPREHENSIVE` 启用**: 堆完整毒化，可检测越界/释放后使用等 bug（调试完成后建议关闭省 RAM）。
148. **`CONFIG_ESP_WIFI_IRAM_OPT` 关闭**: WiFi IRAM 优化关闭，代码走 flash，减少 IRAM 占用。
149. **`CONFIG_BT_CONTROLLER_SINGLE_MODEM_MODE`**: ESP32-C3 为 BLE-only 芯片，不存在该选项，无需设置。

### 2026-06-06 WiFi osi_funcs version 修复 + deinit 顺序修正

150. **`osi_funcs._version` 显式赋值**:
    - **根因**: `memcpy` 从 `g_wifi_osi_funcs` 拷贝时 version 字段为 0，但 ESP-IDF v6.0 期望 `ESP_WIFI_OS_ADAPTER_VERSION` (8)。
    - **修复**: `wifi_cfg_reserve_dma()` 中 `memcpy` 后强制写入 `s_wifi_osi._version = ESP_WIFI_OS_ADAPTER_VERSION`。
    - `wifi_cfg_release_dma()` 恢复 `s_wifi_osi` 时也保留修正后的 version。

151. **WiFi 去初始化顺序修正**:
    - **根因**: `wifi_cfg_start_sta_only()` 中直接 `esp_wifi_stop()` + `esp_wifi_deinit()` 未提前注销事件回调，导致 `Failed to unregister Rx callbacks` 和 `0x3001` 错误。
    - **修复**: 统一用 `wifi_cfg_deinit()` 做完整清理（stop→注销事件→deinit→netif deinit→释放 DMA 池）。

### 2026-06-06 动态 DMA 池分配/释放

152. **新增 `wifi_cfg_release_dma()`**:
    - 释放 2 个 DMA 缓冲块，恢复 `osi_funcs` 为默认（保留 version 修正），清空原始函数指针缓存。
    - `wifi_cfg_deinit()` 末尾自动调用，WiFi 关闭后 DMA 内存完全归还系统。

153. **`wifi_cfg_reserve_dma()` 支持重复调用**:
    - 开头检查 `s_dma_buf[0] != NULL`，若已分配则先调 `release_dma()` 再重新分配。

154. **所有 WiFi 入口统一调用 `reserve_dma()`**:
    - `wifi_cfg_start_sta_only()`、`wifi_cfg_start_ap_only()`、`wifi_cfg_start()` 开头均加 `wifi_cfg_reserve_dma()`。
    - 确保每次 WiFi 启动前 `version=8` 被强制写入，DMA 池按需分配。

### 2026-06-06 堆损坏修复 — DMA 池缓冲区增大至 2048

155. **`CORRUPT HEAP: Bad tail` 修复**:
    - **根因**: WiFi 驱动在 static RX 缓冲区写越界。池分配 1600 字节，驱动实际需要更多（如描述符头部），写入超过 1600 破坏堆尾标记。
    - **修复**: `WIFI_DMA_BUF_SIZE` 从 1600 → 2048，`pool_try_alloc` 匹配范围 `[1600, 2048]`。每个缓冲区多 448 字节安全余量，仅多耗 896 字节堆内存。


### 2026-06-06 `0xFA` 占线锁识别 + EPD 栈缓冲 + BLE 完整释放重建

156. **`img_ctrl_write()` 识别 `0xFA` 占线锁**:
    - **根因**: 网页端发送 `0xFA` 到控制特征值，但 `img_ctrl_write()` 无对应 case → 刷屏 `unknown cmd: 0xFA` 警告。
    - **修复**: `ble_img_rx.h` 枚举新增 `IMG_CMD_FAKE_BUSY = 0xFA`，`img_ctrl_write()` 添加对应 case 仅设 `g_busy = true` 不打印警告。

157. **EPD 4KB 静态 DMA 缓冲 → 栈缓冲 512 字节**:
    - **根因**: EPD 初始化时 `heap_caps_malloc(4096, MALLOC_CAP_DMA)` 常驻 4KB DMA 内存，叠加 WiFi DMA 池后导致 BLE init `Malloc failed` 和 SPI `priv TX buffer` 分配失败。
    - **修复**: 删除 `s_epd_chunk` 静态缓冲和互斥锁。`epd_update_full_from_flash()` 改用栈数组 `uint8_t chunk[512]` 分段读写，零堆分配。ESP32-C3 任务栈 8KB 绰绰有余，SPI DMA 可直接使用栈地址。

158. **`epd_deinit()` 不再释放 DMA 缓冲**: 清理 `epd_init_shared_bus()` 和 `epd_deinit()` 中的 chunk 分配/释放代码，`epd_deinit()` 仅卸载 SPI 设备句柄。

159. **`main.c` 删除 EPD 强行释放**: 移除 `epd_app_init()` 后的 `epd_deinit()` + `LOG_MEM_AFTER` 调用，EPD 常驻 SPI 设备句柄，后续刷屏正常。

160. **`ble_img_pause()` 扩展为完整释放 BLE**:
    - 在原有 `ble_img_deinit()` 之后增加 `esp_bt_controller_disable()` + `esp_bt_controller_deinit()`，释放 32KB 连续 EM 内存。
    - `g_bt_ctrl_inited = false`，`g_ble_init_ok = false`。

161. **`ble_img_resume()` 完整重建 BLE**:
    - 新增 `g_ready_cb_backup` 静态变量，`ble_img_init()` 时备份回调指针。
    - `ble_img_resume()` 执行 `ble_img_ctrl_init()` → `ble_img_init(g_ready_cb_backup)` 完整重建控制器 + host + GATT + 广播。

162. **`ble_adv_stop()` 同步信号量版**:
    - 新增 `g_adv_stop_sem` 信号量，`ADV_COMPLETE` 事件中 `xSemaphoreGive`。
    - `ble_adv_stop()` 等待最多 1000ms 确认停止完成，超时仅记 `ESP_LOGI` 不报错。

### 2026-06-06 SPI DMA 空指针保护 + NimBLE mbuf 扩容

163. **W25Q64 SPI DMA 动态分配 NULL 检查**:
    - `w25q64_spi_txrx()` 中大传输 fallback `heap_caps_malloc` 失败时打印 `SPI DMA alloc %d failed` 并返回 `ESP_ERR_NO_MEM`，杜绝空指针崩溃。

164. **NimBLE mbuf 池扩容** (`sdkconfig.defaults`):
    - `MSYS_1_BLOCK_COUNT`: 8 → 16
    - `MSYS_2_BLOCK_COUNT`: 16 → 24
    - `ACL_BUF_COUNT`: — → 24
    - `ACL_BUF_SIZE`: — → 255
    - 缓解广告/连接时 mbuf 瞬时不足导致的 `adv_start failed: 6`。
