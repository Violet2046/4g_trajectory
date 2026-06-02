### 2.3 BMI160 模块 (六轴 IMU)
- **通信接口**: I2C (ESP-IDF v6 `i2c_new_master_bus`)。
- **中断控制**: INT1 (GPIO 9) 配置为上升沿触发，绑定 **AnyMotion** 运动唤醒；INT2 (GPIO 0) 配置为双击检测 → IMG_RECEIVE。
- **低功耗唤醒**: GPIO 9（即 BMI160 INT1）同时配置为唤醒源，PM tickless-idle 自动进入 light sleep。

### 2.8 主程序工作流（当前状态机）
```raw
INIT → ACTIVE → SAMPLE → ACTIVE
         ↕ (5s无运动)   ↕ (双击)
     LOW_POWER       IMG_RECEIVE
```
#### 状态说明

| 状态 | 行为 |
|------|------|
| **STATE_INIT** | `hardware_init()`: 初始化 CT511N、sensor_hub、W25Q64、gptimer、ISR |
| **STATE_ACTIVE** | 等待 1s 定时器采样、AnyMotion 中断、或 BMI160 双击 (INT2 → IMG_RECEIVE) |
| **STATE_LOW_POWER** | 传感器休眠、PM tickless-idle 自动 light sleep、信号量 + 运动 ISR 唤醒 |
| **STATE_SAMPLE** | 唤醒 CT511N → GPS → 传感器 → 存 Flash (或 RAM 缓存) → 上传 |
| **STATE_IMG_RECEIVE** | BLE 广播 → 手机连接 → 接收图像写入 Flash 后半区 → 自动显示 → 退出 |
```raw
4g_trajectory/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt          # REQUIRES: ... ST7789 ble_img_rx
│   ├── main.c                  # 状态机 (~350行)
│   ├── app_utils.h/.c          # 工具函数 + IMG_RECEIVE 辅助
│   ├── config.h                # 所有可配置参数统一入口（引脚、时序、网络）
│   ├── storage_mgr.h/.c        # 双分区 + RAM Cache
├── components/
│   ├── CT511N/                 # 4G+GPS (UART)
│   ├── BMI160/                 # 6轴 IMU (I2C)
│   ├── AK09911C/               # 3轴磁力计 (I2C)
│   ├── W25Q64/                 # NOR Flash (SPI, 已启用)
│   ├── sensor_hub/             # 传感器管理中间层
│   ├── ST7789/                 # 240×240 LCD 驱动 (SPI)
│   ├── ble_img_rx/             # BLE 图像接收服务 (NimBLE)
│   └── wifi_cfg/               # WiFi 配网 + TCP 发送
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