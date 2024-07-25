#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
    #include "task.h"
}


BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                        const char * const pcName,    /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
                        const configSTACK_DEPTH_TYPE usStackDepth,
                        void * const pvParameters,
                        BaseType_t uxPriority,
                        TaskHandle_t * const pxCreatedTask ){
    return mock().actualCall(__FUNCTION__)
        .withParameter("pxTaskCode", pxTaskCode)
        .withParameter("pcName", pcName)
        .withParameter("usStackDepth", usStackDepth)
        .withParameter("pvParameters", pvParameters)
        .withParameter("uxPriority", uxPriority)
        .withOutputParameter("pxCreatedTask", pxCreatedTask)
        .returnIntValue();
}

