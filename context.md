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
- **中断控制**: INT1 (GPIO 20) 配置为上升沿触发，绑定 **AnyMotion** 运动唤醒；INT2 (GPIO 0) 配置为双击检测。
- **Light-sleep 唤醒**: GPIO 20（即 BMI160 INT1）同时配置为 Light-sleep 唤醒源。

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
```raw
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
```raw
4g_trajectory/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt          # REQUIRES: ... ST7789 ble_img_rx
│   ├── main.c                  # 状态机 (~350行)
│   ├── app_utils.h/.c          # 工具函数 + IMG_RECEIVE 辅助
│   ├── pin_config.h            # 所有引脚定义统一入口
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