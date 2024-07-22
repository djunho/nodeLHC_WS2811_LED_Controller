#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>
#include "FreeRTOS.h"

typedef void (*TaskFunction_t)( void * );
#define configSTACK_DEPTH_TYPE uint16_t
typedef void * TaskHandle_t;

static inline BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                        const char * const pcName,    /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
                        const configSTACK_DEPTH_TYPE usStackDepth,
                        void * const pvParameters,
                        BaseType_t uxPriority,
                        TaskHandle_t * const pxCreatedTask ){
    (void)pxTaskCode;
    (void)pcName;
    (void)usStackDepth;
    (void)pvParameters;
    (void)uxPriority;
    (void)pxCreatedTask;
    return 0;
}

static inline void vTaskDelete( TaskHandle_t xTaskToDelete ){
    (void)xTaskToDelete;
}

#endif // _TASK_H_

