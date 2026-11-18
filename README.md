# platform_application

应用通过 `#include "project_config.h"` 统一引入标准库、芯片接口、RTT 和 RTOS 接口，使用板级功能时另行包含 `bsp_gpio.h`。使用方需提供主项目 `board/config`、`board/bsp` 及现有芯片和 RTT 包含路径。RTT Viewer 是电脑端工具，固件使用配置头引入的 RTT 通信及日志接口。

`main.c` 启动时先调用 `hardware_init()` 配置 MPU、缓存、HAL 和系统时钟，再初始化 RTT 和板级 GPIO，输出启动信息并调用 `task_start()`。时钟配置失败时，程序停在 `hardware_init()` 内。

`task/task_manage.c/.h` 管理任务创建、调度器启动和 RTOS 错误钩子。`task_start()` 在初始化完成后调用一次，不返回。

主项目 `board/config/task_config.h` 保存任务优先级。`task_manage.c` 通过弱引用创建可选心跳任务，关闭 `GCC_HEARTBEAT` 时跳过该任务，调度器仍可启动。

本仓 CMakeLists.txt 仅提供 application::main、application::task_manage、application::heartbeat 源码接口，最终由主仓 build/gcc 决定是否链接。INTERFACE 源码进入最终固件目标，可选任务不会因普通静态库的按需提取而丢失。增加其他可选任务时，在本仓声明对应接口，并由主仓构建后端选择。此机制针对当前 ARM GCC/ELF 工具链。

`heart_beat/heartbeat.h` 向任务层提供空的初始化入口 `heartbeat_init()` 和周期入口 `heartbeat_process(period)`。状态灯沿用 GPIO 初始电平，随后按累计周期翻转。计时状态保存在 `system_runningLed()` 内部，默认累计每 500 ms 翻转 `MCU_LED`，每 1000 ms 翻转 `MCU_LED1`。

`task/task_heartbeat.c/.h` 负责 FreeRTOS 任务创建、栈深度和周期。启动时读取一次 tick 作为 `xTaskDelayUntil()` 的调度基准。每次唤醒后将 `HEARTBEAT_TASK_PERIOD_MS`（默认 50 ms）传给状态灯接口。任务优先级由 `board/config/task_config.h` 设置。

主仓 `GCC_HEARTBEAT=AUTO` 默认启用心跳，也可显式选择 ON 或 OFF。开启时 `application::heartbeat` 同时编入状态灯实现和任务层。

应用提供内存分配失败和栈溢出钩子。SysTick 由 `system_init.c` 接入，SVC 和 PendSV 由选中的 FreeRTOS 移植层提供，HardFault 使用 STM32 启动文件的默认处理。

`hardware_init()` 位于 `platform_common/core/stm32f7xx/system_init.c`；构建时加入该源文件，并将其所在目录加入包含路径。

以太网初始化、网络轮询及 IP/端口等应用配置已移出当前代码；修改前的完整文件保存在主仓库外的暂存备份中。

不提供自定义 C 库系统调用适配。后续按需使用标准库，具体输入输出和堆内存支持由工具链、运行库及链接配置确定。日志使用 `platform_common/RTT/RTT/SEGGER_RTT.h` 中的 `RLOG_INFO`（绿色）、`RLOG_ERR`（红色）、`RLOG_WAR`（黄色），自动换行；构建时使用 `platform_common/RTT/RTT` 和 `platform_common/RTT/Config` 包含路径。

日志文本统一使用 ASCII 标点。`RLOG_PRINT(...)` 提供格式化纯文本行输出，不添加等级前缀或颜色，不受 `RTT_LOG_LEVEL` 过滤；关闭 `PROJECT_USE_RTT` 时编译为空操作。普通日志继续使用 `RLOG_INFO`、`RLOG_WAR`、`RLOG_ERR`，保留 `[INFO]`、`[WAR]`、`[ERR]` 前缀、等级过滤及可选颜色。两类接口均自动追加 `\r\n`，调用处不再添加换行。开机信息使用 `RLOG_PRINT`，以 40 个 `-` 作为上下分隔线，字段按 `Project :`、`Version :`、`Build   :` 对齐。项目名称、版本、编译日期和时间统一取自主项目 `board/config/version.h`，日期时间使用编译器的 `__DATE__`、`__TIME__`；心跳任务不打印启动信息。

应用依赖主项目 BSP、board/config、common::stm32f7xx 和 FreeRTOS。主仓固定启用 FreeRTOS，并通过 `PROJECT_USE_RTT` 控制日志接口。
