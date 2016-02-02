/*
 * Race Capture Firmware
 *
 * Copyright (C) 2015 Autosport Labs
 *
 * This file is part of the Race Capture firmware suite
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details. You should
 * have received a copy of the GNU General Public License along with
 * this code. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * RaceCapture Pro main
 *
 * NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
 *        The processor MUST be in supervisor mode when vTaskStartScheduler is
 *        called.  The demo applications included in the FreeRTOS.org download switch
 *        to supervisor mode prior to main being called.  If you are not using one of
 *        these demo application projects then ensure Supervisor mode is used.
 */

#include "FreeRTOS.h"
#include "LED.h"
#include "OBD2_task.h"
#include "capabilities.h"
#include "connectivityTask.h"
#include "constants.h"
#include "cpu.h"
#include "fileWriter.h"
#include "gpioTasks.h"
#include "gpsTask.h"
#include "loggerHardware.h"
#include "loggerTaskEx.h"
#include "luaScript.h"
#include "luaTask.h"
#include "messaging.h"
#include "printk.h"
#include "task.h"
#include "usb_comm.h"
#include "watchdog.h"

#include <app_info.h>
#include <stdbool.h>

#define FATAL_ERROR_SCHEDULER	1
#define FATAL_ERROR_HARDWARE	2
#define FLASH_PAUSE_DELAY 	5000000
#define FLASH_DELAY 		1000000

__attribute__((aligned (4)))
static const struct app_info_block info_block = {
    .magic_number = APP_INFO_MAGIC_NUMBER,
    .info_crc = 0xDEADBEEF,
};

static void fatalError(int type)
{
        int count;

        switch (type) {
        case FATAL_ERROR_HARDWARE:
                count = 1;
                break;
        case FATAL_ERROR_SCHEDULER:
                count = 2;
                break;
        default:
                count = 3;
                break;
        }

        while(1) {
                for (int c = 0; c < count; c++) {
                        LED_enable(1);
                        LED_enable(2);
                        for (int i=0; i<FLASH_DELAY; i++) {}
                        LED_disable(1);
                        LED_disable(2);
                        for (int i=0; i < FLASH_DELAY; i++) {}
                }
                for (int i=0; i < FLASH_PAUSE_DELAY; i++) {}
        }
}

/*
 * Define task priorities here.  Ensure that these values are
 * all below `configMAX_PRIORITIES - 1` in the FreeRTOSConfig.h.
 * See http://www.freertos.org/RTOS-task-priority.html for more
 * info on how to best set these.  0 is lowest priority.
 */
#define TASK_PRIORITY(v)	(tskIDLE_PRIORITY + v)
#define RCP_INPUT_PRIORITY	TASK_PRIORITY(4)
#define RCP_OUTPUT_PRIORITY	TASK_PRIORITY(3)
#define RCP_LOGGING_PRIORITY	TASK_PRIORITY(2)
#define RCP_LUA_PRIORITY	TASK_PRIORITY(1)

void setupTask(void *delTask)
{
        initialize_tracks();
        initialize_logger_config();
        InitLoggerHardware();
        initMessaging();

        startGPSTask(RCP_INPUT_PRIORITY);
        startOBD2Task(RCP_INPUT_PRIORITY);
        startConnectivityTask(RCP_OUTPUT_PRIORITY);
        startLoggerTaskEx(RCP_LOGGING_PRIORITY);

#if defined(USB_SERIAL_SUPPORT)
        startUSBCommTask(RCP_INPUT_PRIORITY);
#endif

#if GPIO_CHANNELS > 0
        startGPIOTasks(RCP_INPUT_PRIORITY);
#endif

#if defined(SDCARD_SUPPORT)
        startFileWriterTask(RCP_OUTPUT_PRIORITY);
#endif

#if defined(LUA_SUPPORT)
        initialize_script();
        startLuaTask(RCP_LUA_PRIORITY);
#endif

        /* Removes this setup task from the scheduler */
        if (delTask)
                vTaskDelete(NULL);
}

int main( void )
{
        ALWAYS_KEEP(info_block);
        cpu_init();
        pr_info("*** Start! ***\r\n");
        watchdog_init(WATCHDOG_TIMEOUT_MS);

        /*
         * Start the scheduler.
         *
         * NOTE : Tasks run in system mode and the scheduler runs in
         * Supervisor mode. The processor MUST be in supervisor mode
         * when vTaskStartScheduler is called.  The demo applications
         * included in the FreeRTOS.org download switch to supervisor
         * mode prior to main being called.  If you are not using one
         * of these demo application projects then ensure Supervisor
         * mode is used here.
         */

        if (TASK_TASK_INIT) {
                xTaskCreate(setupTask,(signed portCHAR*) "Hardware Init",
                            configMINIMAL_STACK_SIZE + 500, (void *) true,
                            tskIDLE_PRIORITY, NULL);
        } else {
                setupTask((void *) false);
        }

        vTaskStartScheduler();
        fatalError(FATAL_ERROR_SCHEDULER);

        return 0;
}

/*-----------------------------------------------------------*/
void vApplicationMallocFailedHook(void)
{
        /*
         * vApplicationMallocFailedHook() will only be called if
         * configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It
         * is a hook function that will get called if a call to pvPortMalloc()
         * fails. pvPortMalloc() is called internally by the kernel whenever a
         * task, queue, timer or semaphore is created.  It is also called by
         * various parts of the demo application.  If heap_1.c or heap_2.c are
         * used, then the size of the heap available to pvPortMalloc() is
         * defined by configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the
         * xPortGetFreeHeapSize() API function can be used to query the size
         * of free heap space that remains (although it does not provide
         * information on how the remaining heap might be fragmented).
         */
        taskDISABLE_INTERRUPTS();
        for(;;);
}

/*-----------------------------------------------------------*/
void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName)
{
        (void) pcTaskName;
        (void) pxTask;

        /*
         * Run time stack overflow checking is performed if
         * configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.
         * This hook function is called if a stack overflow is detected.
         */
        taskDISABLE_INTERRUPTS();
        for(;;);
}
