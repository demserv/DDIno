// @requirement RF-PEND-001, RF-PEND-002, RB-PEND-001..003 Ações pendentes
#include "pending_actions.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "pend_act";
#define MAX_PENDING 16

typedef struct {
    pending_action_type_t type;
    uint8_t data[64];
    uint32_t len;
    bool used;
} pending_entry_t;

static pending_entry_t s_queue[MAX_PENDING];

esp_err_t pending_actions_init(void)
{
    memset(s_queue, 0, sizeof(s_queue));
    ESP_LOGI(TAG, "Pending actions initialized");
    return ESP_OK;
}

esp_err_t pending_actions_enqueue(pending_action_type_t type, const uint8_t *data, uint32_t len)
{
    for (int i = 0; i < MAX_PENDING; i++) {
        if (!s_queue[i].used) {
            s_queue[i].type = type;
            s_queue[i].len = (len > 64) ? 64 : len;
            if (data && s_queue[i].len > 0) memcpy(s_queue[i].data, data, s_queue[i].len);
            s_queue[i].used = true;
            return ESP_OK;
        }
    }
    return ESP_ERR_NO_MEM;
}

esp_err_t pending_actions_dequeue(pending_action_type_t *type, uint8_t *data, uint32_t *len)
{
    if (!type || !len) return ESP_ERR_INVALID_ARG;
    for (int i = 0; i < MAX_PENDING; i++) {
        if (s_queue[i].used) {
            *type = s_queue[i].type;
            if (data) memcpy(data, s_queue[i].data, s_queue[i].len);
            *len = s_queue[i].len;
            s_queue[i].used = false;
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

int pending_actions_count(void)
{
    int c = 0;
    for (int i = 0; i < MAX_PENDING; i++) { if (s_queue[i].used) c++; }
    return c;
}

esp_err_t pending_actions_process_all(void)
{
    pending_action_type_t t;
    uint8_t buf[64];
    uint32_t l;
    int processed = 0;
    while (pending_actions_dequeue(&t, buf, &l) == ESP_OK) {
        ESP_LOGI(TAG, "Processed pending action type %d", (int)t);
        processed++;
    }
    ESP_LOGI(TAG, "Processed %d pending actions", processed);
    return ESP_OK;
}
