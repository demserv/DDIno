#ifndef FIRMWARE_INCLUDE_TASK_MANAGER_H
#define FIRMWARE_INCLUDE_TASK_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TASK_ID_SAFETY_CORE    0
#define TASK_ID_SENSORS        1
#define TASK_ID_PLUG_CONTROL   2
#define TASK_ID_STORAGE        3
#define TASK_ID_UI             4
#define TASK_ID_WEB            5
#define TASK_ID_DIAG           6
#define TASK_ID_COUNT          7

#define TASK_PRI_SAFETY_CORE   (10U)
#define TASK_PRI_SENSORS       (9U)
#define TASK_PRI_PLUG_CONTROL  (8U)
#define TASK_PRI_STORAGE       (6U)
#define TASK_PRI_UI            (5U)
#define TASK_PRI_WEB           (4U)
#define TASK_PRI_DIAG          (2U)

#define TASK_STACK_SAFETY_CORE   (4096U)
#define TASK_STACK_SENSORS       (4096U)
#define TASK_STACK_PLUG_CONTROL  (4096U)
#define TASK_STACK_STORAGE       (4096U)
#define TASK_STACK_UI            (8192U)
#define TASK_STACK_WEB           (8192U)
#define TASK_STACK_DIAG          (3072U)

#define TASK_CORE_SAFETY_CORE   (1)
#define TASK_CORE_SENSORS       (1)
#define TASK_CORE_PLUG_CONTROL  (1)
#define TASK_CORE_STORAGE       (1)
#define TASK_CORE_UI            (0)
#define TASK_CORE_WEB           (0)
#define TASK_CORE_DIAG          (0)

#define TASK_NAME_SAFETY_CORE   "safety_core"
#define TASK_NAME_SENSORS       "sensors"
#define TASK_NAME_PLUG_CONTROL  "plug_control"
#define TASK_NAME_STORAGE       "storage"
#define TASK_NAME_UI            "ui"
#define TASK_NAME_WEB           "web"
#define TASK_NAME_DIAG          "diag"

#define TASK_PERIOD_MS_SAFETY_CORE   (50U)
#define TASK_PERIOD_MS_SENSORS       (200U)
#define TASK_PERIOD_MS_PLUG_CONTROL  (100U)
#define TASK_PERIOD_MS_STORAGE       (1000U)
#define TASK_PERIOD_MS_UI            (10U)
#define TASK_PERIOD_MS_WEB           (50U)
#define TASK_PERIOD_MS_DIAG          (1000U)

#define TASK_WDT_TIMEOUT_MS_SAFETY_CORE   (2000U)
#define TASK_WDT_TIMEOUT_MS_SENSORS       (3000U)
#define TASK_WDT_TIMEOUT_MS_PLUG_CONTROL  (2000U)
#define TASK_WDT_TIMEOUT_MS_STORAGE       (5000U)
#define TASK_WDT_TIMEOUT_MS_UI            (5000U)
#define TASK_WDT_TIMEOUT_MS_WEB           (10000U)
#define TASK_WDT_TIMEOUT_MS_DIAG          (10000U)

typedef struct {
    int task_id;
    const char *name;
    UBaseType_t priority;
    uint32_t stack_size;
    BaseType_t core_id;
    uint32_t period_ms;
    uint32_t wdt_timeout_ms;
    bool heartbeat_required;
    TaskHandle_t handle;
} task_definition_t;

esp_err_t task_manager_register_fn(int task_id, TaskFunction_t task_fn);
esp_err_t task_manager_launch_all(void);
esp_err_t task_manager_create(TaskFunction_t task_fn, int task_id);
TaskHandle_t task_manager_get_handle(int task_id);
const char *task_manager_get_name(int task_id);
int task_manager_get_id_by_name(const char *name);
const task_definition_t *task_manager_get_definition(int task_id);

#endif
