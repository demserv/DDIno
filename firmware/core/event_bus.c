// @requirement RF-GLOBAL-002 Barramento de eventos para comunicação desacoplada
// @requirement RF-UI-INPUT-001 Eventos de UI e sistema roteados via event_bus
#include "event_bus.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "event_bus";

#define MAX_SUBSCRIBERS_PER_EVENT 8
#define MAX_PENDING_EVENTS 16

typedef struct {
    event_id_t evt;
    event_callback_t cb;
    void *user_ctx;
    bool active;
} subscriber_t;

typedef struct {
    event_id_t evt;
    void *data;
} pending_event_t;

static subscriber_t s_subs[MAX_SUBSCRIBERS_PER_EVENT];
static int s_sub_count;

static pending_event_t s_pending[MAX_PENDING_EVENTS];
static int s_pending_head;
static int s_pending_tail;

esp_err_t event_bus_init(void)
{
    memset(s_subs, 0, sizeof(s_subs));
    s_sub_count = 0;
    s_pending_head = 0;
    s_pending_tail = 0;
    ESP_LOGI(TAG, "Event bus initialized");
    return ESP_OK;
}

esp_err_t event_bus_subscribe(event_id_t evt, event_callback_t cb, void *user_ctx)
{
    if (s_sub_count >= MAX_SUBSCRIBERS_PER_EVENT) return ESP_ERR_NO_MEM;
    s_subs[s_sub_count].evt = evt;
    s_subs[s_sub_count].cb = cb;
    s_subs[s_sub_count].user_ctx = user_ctx;
    s_subs[s_sub_count].active = true;
    s_sub_count++;
    return ESP_OK;
}

esp_err_t event_bus_unsubscribe(event_id_t evt, event_callback_t cb)
{
    for (int i = 0; i < s_sub_count; i++) {
        if (s_subs[i].evt == evt && s_subs[i].cb == cb) {
            s_subs[i].active = false;
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t event_bus_publish(event_id_t evt, void *data)
{
    for (int i = 0; i < s_sub_count; i++) {
        if (s_subs[i].active && s_subs[i].evt == evt) {
            s_subs[i].cb(evt, data, s_subs[i].user_ctx);
        }
    }
    return ESP_OK;
}

esp_err_t event_bus_publish_isr(event_id_t evt, void *data)
{
    int next = (s_pending_head + 1) % MAX_PENDING_EVENTS;
    if (next == s_pending_tail) return ESP_ERR_NO_MEM;
    s_pending[s_pending_head].evt = evt;
    s_pending[s_pending_head].data = data;
    s_pending_head = next;
    return ESP_OK;
}

void event_bus_process_pending(void)
{
    while (s_pending_tail != s_pending_head) {
        event_bus_publish(s_pending[s_pending_tail].evt, s_pending[s_pending_tail].data);
        s_pending_tail = (s_pending_tail + 1) % MAX_PENDING_EVENTS;
    }
}
