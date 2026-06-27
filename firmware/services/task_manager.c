#include "task_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "task_mgr";

static TaskHandle_t s_handles[TASK_ID_COUNT];

static const task_definition_t s_tasks[TASK_ID_COUNT] = {
    {
        .task_id = TASK_ID_SAFETY_CORE,
        .name = TASK_NAME_SAFETY_CORE,
        .priority = TASK_PRI_SAFETY_CORE,
        .stack_size = TASK_STACK_SAFETY_CORE,
        .core_id = TASK_CORE_SAFETY_CORE,
        .period_ms = TASK_PERIOD_MS_SAFETY_CORE,
        .wdt_timeout_ms = TASK_WDT_TIMEOUT_MS_SAFETY_CORE,
        .heartbeat_required = true,
        .handle = NULL
    },
    {
        .task_id = TASK_ID_SENSORS,
        .name = TASK_NAME_SENSORS,
        .priority = TASK_PRI_SENSORS,
        .stack_size = TASK_STACK_SENSORS,
        .core_id = TASK_CORE_SENSORS,
        .period_ms = TASK_PERIOD_MS_SENSORS,
        .wdt_timeout_ms = TASK_WDT_TIMEOUT_MS_SENSORS,
        .heartbeat_required = true,
        .handle = NULL
    },
    {
        .task_id = TASK_ID_PLUG_CONTROL,
        .name = TASK_NAME_PLUG_CONTROL,
        .priority = TASK_PRI_PLUG_CONTROL,
        .stack_size = TASK_STACK_PLUG_CONTROL,
        .core_id = TASK_CORE_PLUG_CONTROL,
        .period_ms = TASK_PERIOD_MS_PLUG_CONTROL,
        .wdt_timeout_ms = TASK_WDT_TIMEOUT_MS_PLUG_CONTROL,
        .heartbeat_required = true,
        .handle = NULL
    },
    {
        .task_id = TASK_ID_STORAGE,
        .name = TASK_NAME_STORAGE,
        .priority = TASK_PRI_STORAGE,
        .stack_size = TASK_STACK_STORAGE,
        .core_id = TASK_CORE_STORAGE,
        .period_ms = TASK_PERIOD_MS_STORAGE,
        .wdt_timeout_ms = TASK_WDT_TIMEOUT_MS_STORAGE,
        .heartbeat_required = true,
        .handle = NULL
    },
    {
        .task_id = TASK_ID_UI,
        .name = TASK_NAME_UI,
        .priority = TASK_PRI_UI,
        .stack_size = TASK_STACK_UI,
        .core_id = TASK_CORE_UI,
        .period_ms = TASK_PERIOD_MS_UI,
        .wdt_timeout_ms = TASK_WDT_TIMEOUT_MS_UI,
        .heartbeat_required = true,
        .handle = NULL
    },
    {
        .task_id = TASK_ID_WEB,
        .name = TASK_NAME_WEB,
        .priority = TASK_PRI_WEB,
        .stack_size = TASK_STACK_WEB,
        .core_id = TASK_CORE_WEB,
        .period_ms = TASK_PERIOD_MS_WEB,
        .wdt_timeout_ms = TASK_WDT_TIMEOUT_MS_WEB,
        .heartbeat_required = true,
        .handle = NULL
    },
    {
        .task_id = TASK_ID_DIAG,
        .name = TASK_NAME_DIAG,
        .priority = TASK_PRI_DIAG,
        .stack_size = TASK_STACK_DIAG,
        .core_id = TASK_CORE_DIAG,
        .period_ms = TASK_PERIOD_MS_DIAG,
        .wdt_timeout_ms = TASK_WDT_TIMEOUT_MS_DIAG,
        .heartbeat_required = false,
        .handle = NULL
    }
};

static TaskFunction_t s_task_fns[TASK_ID_COUNT];

esp_err_t task_manager_register_fn(int task_id, TaskFunction_t task_fn)
{
    if (task_id < 0 || task_id >= TASK_ID_COUNT) return ESP_ERR_INVALID_ARG;
    s_task_fns[task_id] = task_fn;
    return ESP_OK;
}

esp_err_t task_manager_launch_all(void)
{
    memset(s_handles, 0, sizeof(s_handles));

    for (int i = 0; i < TASK_ID_COUNT; i++) {
        const task_definition_t *def = &s_tasks[i];
        ESP_LOGI(TAG, "Task %d (%s): pri=%u stack=%u core=%d",
                 def->task_id, def->name,
                 (unsigned)def->priority,
                 (unsigned)def->stack_size,
                 (int)def->core_id);
        if (s_task_fns[i] != NULL) {
            task_manager_create(s_task_fns[i], i);
        }
    }

    return ESP_OK;
}

esp_err_t task_manager_create(TaskFunction_t task_fn, int task_id)
{
    if (task_id < 0 || task_id >= TASK_ID_COUNT) return ESP_ERR_INVALID_ARG;
    if (!task_fn) return ESP_ERR_INVALID_ARG;
    if (s_handles[task_id] != NULL) return ESP_ERR_INVALID_STATE;

    const task_definition_t *def = &s_tasks[task_id];
    BaseType_t ret = xTaskCreatePinnedToCore(
        task_fn,
        def->name,
        def->stack_size,
        NULL,
        def->priority,
        &s_handles[task_id],
        def->core_id
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar task %s", def->name);
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "Task %s criada (pri=%u stack=%u core=%d)",
             def->name, (unsigned)def->priority,
             (unsigned)def->stack_size, (int)def->core_id);
    return ESP_OK;
}

TaskHandle_t task_manager_get_handle(int task_id)
{
    if (task_id < 0 || task_id >= TASK_ID_COUNT) return NULL;
    return s_handles[task_id];
}

const char *task_manager_get_name(int task_id)
{
    if (task_id < 0 || task_id >= TASK_ID_COUNT) return NULL;
    return s_tasks[task_id].name;
}

int task_manager_get_id_by_name(const char *name)
{
    if (!name) return -1;
    for (int i = 0; i < TASK_ID_COUNT; i++) {
        if (strcmp(name, s_tasks[i].name) == 0) return i;
    }
    return -1;
}

const task_definition_t *task_manager_get_definition(int task_id)
{
    if (task_id < 0 || task_id >= TASK_ID_COUNT) return NULL;
    return &s_tasks[task_id];
}
