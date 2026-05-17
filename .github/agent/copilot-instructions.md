---
description: ESP32-C3 + ESP-IDF v6 项目通用规范 — BMI160 + CT511N 轨迹追踪
globs:
alwaysApply: true
---

# 项目通用规范

## 文件含义
- `context.md`：项目上下文，记录项目历史背景、需求和限制。**每次修改代码后必须更新**。
- `待办事项.txt`：项目待办事项列表。

## 技术栈
- **主控**: ESP32-C3 (RISC-V)
- **框架**: ESP-IDF v6.0
- **RTOS**: FreeRTOS
- **通信**: I2C (master_bus v6 API), UART (esp_driver_uart)

## 通用开发原则

### 1. ESP-IDF v6 API 优先
- **禁止**使用 ESP-IDF v5.x 或更旧的遗留 API。
- I2C → `i2c_master_bus_handle_t` + `i2c_master_transmit_receive()`（非 `i2c_cmd_link_create`）。
- GPIO ISR 回调类型 → `gpio_isr_t`（非已废弃的 `gpio_isr_handler_t`）。
- 中断服务函数必须添加 `IRAM_ATTR` 属性。

### 2. CMake 依赖声明
- 组件 `REQUIRES` 必须使用 ESP-IDF v6 拆分后的组件名：
  - `driver/i2c_master.h` → `REQUIRES esp_driver_i2c`
  - `driver/gpio.h` → `REQUIRES esp_driver_gpio`
  - `driver/uart.h` → `REQUIRES esp_driver_uart`
- **禁止**使用旧的 `REQUIRES "driver"`。

### 3. 中断寄存器配置需对照数据手册（BMI160 相关）
- BMI160 INT_MAP 寄存器语义：**"set bit = 路由到该寄存器对应的引脚"**。
  - `INT_MAP[0]` (0x55) → INT1；`INT_MAP[2]` (0x57) → INT2。
  - 非 "0=INT1 / 1=INT2"。
- INT_OUT_CTRL 必须明确设置 `int1/2_lvl` 与 `int1/2_output_en`。

### 4. 零警告编译
- 项目启用 `-Werror`，**所有警告视同错误**。
- 未初始化变量、未使用变量等必须修复，不能忽略。
- 提交前必须确保 `idf.py build` 无报错。

### 5. 驱动层与应用层分离
- 传感器配置、寄存器操作封装在驱动源文件中（`BMI160.c`）。
- ISR 回调函数、信号量创建、业务逻辑放在应用层（`main.c`）。

### 6. context.md 同步更新
- **每次功能性代码变更后**，必须在 `context.md` 的开发排雷记录中追加说明。
- 新模块或新功能需在文档中补充架构描述。

### 7. I2C 外设驱动模式（BMI160 参考）
- 使用 `reg_read(handle, reg, &data, len)` / `reg_write(handle, reg, value)` 模式封装 I2C 寄存器操作。
- 分层的 handle 结构（config → bus_handle → dev_handle → lock）。
- 创建时验证 chip_id，失败自动释放资源回滚。

### 8. 代码风格
- 注释统一使用**英文**。
- 缩进、命名风格遵循已有代码（Tab 缩进，snake_case 命名，`{` 换行）。
- 函数名以模块前缀开头（如 `bmi160_`、`ct511n_`）。
- 公开函数加 Doxygen 注释 `@brief` `@param` `@return`。