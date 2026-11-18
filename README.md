# platform_application

应用通过 `#include "project_config.h"` 统一引入标准库、芯片接口、RTT 和 RTOS 接口，使用板级功能时另行包含 `board.h`。使用方需提供主项目 `board/config`、`board/bsp` 及现有芯片和 RTT 包含路径。RTT Viewer 是电脑端工具，固件使用配置头引入的 RTT 通信及日志接口。

`main.c` 保留应用入口。启动时先调用 `hardware_init()` 初始化芯片支持的缓存和 HAL，再初始化 RTT、板级 LED 和时钟，最后调用 `task_start()`。

`task/task_manage.c/.h` 管理任务创建、调度器启动和 RTOS 错误钩子。`task_start()` 在初始化完成后调用一次，不返回。

主项目 `board/config/task_config.h` 仅集中保存任务优先级。`task_manage.c` 只包含自己的头文件和 `task_config.h`，不依赖具体任务的私有头文件；任务创建函数仅在管理文件中声明为 GCC 弱引用，实际实现仍为普通强符号；管理模块仅向创建函数传入优先级。`task_create_optional()` 检查入口：已链接则创建，未链接则输出 WAR 并跳过；没有应用任务时仍可启动调度器运行内核任务。因此可以只从 CMake 固件源列表中移除 `task/task_heartbeat.c`，无需再添加 C 宏开关。管理模块不要求被排除任务的私有头文件存在，但共享的 task_config.h 必须保留。

本仓 CMakeLists.txt 仅提供 application::main、application::task_manage、application::heartbeat 源码接口，最终由主仓 build/gcc 决定是否链接。INTERFACE 源码进入最终固件目标，可选任务不会因普通静态库的按需提取而丢失。增加其他可选任务时，在本仓声明对应接口，并由主仓构建后端选择。此机制针对当前 ARM GCC/ELF 工具链。

`task/task_heartbeat.c` 实现 `heartBeat_process()` 和 `heartBeat_create(priority)`；名称、栈深度、周期及接口声明均位于 `task/task_heartbeat.h`。当前每 500 ms 翻转 LED0、每 1000 ms 翻转 LED1，栈为 512 个 32 位字（2 KiB），优先级由 task_config.h 的 `HEARTBEAT_TASK_PRIORITY` 指定，默认 idle+1。构建时加入 `main.c`、`task/task_manage.c`，按需加入 `task/task_heartbeat.c`。现有 `board/config` 包含路径即可找到任务配置，无需增加路径。

任务名称、栈、周期和启动延时直接在 task/task_heartbeat.h 中修改；优先级仍统一由 board/config/task_config.h 设置。主仓 GCC_HEARTBEAT=AUTO 随 FreeRTOS 启用，也可显式 ON/OFF 覆盖；最终源码选择在 build/gcc，管理模块通过弱引用兼容排除任务。

心跳任务使用 xTaskDelayUntil 固定周期调度，默认任务周期 50 ms，LED0 通过 HEARTBEAT_LED_TOGGLE_INTERVAL_MS 设置为每 500 ms 翻转，LED1 通过 HEARTBEAT_LED1_TOGGLE_INTERVAL_MS 设置为每 1000 ms 翻转；HEARTBEAT_TASK_START_DELAY_MS 默认 0，表示任务开始后立即执行第一次 LED 翻转。非零值先延时，再建立固定周期基准。周期调度不会消除任务超时或抢占造成的延迟。

应用提供内存分配失败、栈溢出钩子，失败时停止运行等待调试；SysTick/HardFault 位于芯片 `system_init.c`，SVC/PendSV 位于选中的 RTOS 移植层。

`hardware_init()` 位于 `platform_common/core/stm32f7xx/system_init.c`；构建时加入该源文件，并将其所在目录加入包含路径。

以太网初始化、网络轮询及 IP/端口等应用配置已移出当前代码；修改前的完整文件保存在主仓库外的暂存备份中。

不提供自定义 C 库系统调用适配。后续按需使用标准库，具体输入输出和堆内存支持由工具链、运行库及链接配置确定。日志使用 `platform_common/RTT/RTT/SEGGER_RTT.h` 中的 `RLOG_INFO`（绿色）、`RLOG_ERR`（红色）、`RLOG_WAR`（黄色），自动换行；构建时使用 `platform_common/RTT/RTT` 和 `platform_common/RTT/Config` 包含路径。

日志文本统一使用 ASCII 标点。`RLOG_PRINT(...)` 提供格式化纯文本行输出，不添加等级前缀或颜色，不受 `RTT_LOG_LEVEL` 过滤；关闭 `PROJECT_USE_RTT` 时编译为空操作。普通日志继续使用 `RLOG_INFO`、`RLOG_WAR`、`RLOG_ERR`，保留 `[INFO]`、`[WAR]`、`[ERR]` 前缀、等级过滤及可选颜色。两类接口均自动追加 `\r\n`，调用处不再添加换行。开机信息使用 `RLOG_PRINT`，以 40 个 `-` 作为上下分隔线，字段按 `Project :`、`Version :`、`Build   :` 对齐。项目名称、版本、编译日期和时间统一取自主项目 `board/config/version.h`，日期时间使用编译器的 `__DATE__`、`__TIME__`；心跳任务不打印启动信息。

应用依赖主项目 BSP、board/config 和 common::stm32f7xx。主仓定义 PROJECT_USE_FREERTOS / PROJECT_USE_RTT 控制 RTOS 与日志接口；裸机入口保持等待中断。已通过完整固件链接及功能关闭组合构建，尚未上板运行。
