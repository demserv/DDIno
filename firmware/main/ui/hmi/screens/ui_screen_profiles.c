// @requirement RF-UI-MENU-002 CRUD de perfis + renomear com teclado livre
#include "ui_screen_profiles.h"
#include "../ui_theme.h"
#include "../ui_screen_manager.h"
#include "../ui_events.h"
#include "../ui_profile_name_editor.h"
#include "profile_manager.h"
#include "esp_err.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *g_list_lbl = NULL;
static lv_obj_t *g_status_lbl = NULL;
static lv_obj_t *g_target_lbl = NULL;
static lv_obj_t *g_screen_root = NULL;
static char s_selected[PROFILE_NAME_MAX] = "default";
static char s_target_name[PROFILE_NAME_MAX] = "reef";
static int s_sel_idx = 0;

static void refresh_list(void)
{
    char names[PROFILE_MANAGER_MAX][PROFILE_NAME_MAX];
    uint8_t count = 0;
    profile_manager_list(names, &count);

    char buf[512];
    buf[0] = '\0';
    for (uint8_t i = 0; i < count; i++) {
        char line[40];
        snprintf(line, sizeof(line), "%s %s\n", (i == (uint8_t)s_sel_idx) ? ">" : " ", names[i]);
        strncat(buf, line, sizeof(buf) - strlen(buf) - 1);
    }
    if (count == 0) {
        snprintf(buf, sizeof(buf), "(nenhum perfil salvo)");
    } else if (s_sel_idx >= (int)count) {
        s_sel_idx = 0;
    }
    if (count > 0) {
        snprintf(s_selected, sizeof(s_selected), "%s", names[s_sel_idx]);
    }
    if (g_list_lbl) {
        lv_label_set_text(g_list_lbl, buf);
    }

    if (g_target_lbl) {
        char tbuf[48];
        snprintf(tbuf, sizeof(tbuf), "Nome alvo: %s", s_target_name);
        lv_label_set_text(g_target_lbl, tbuf);
    }
}

static void save_cb(lv_event_t *e)
{
    (void)e;
    esp_err_t err = profile_manager_save(s_target_name);
    if (err == ESP_OK) {
        snprintf(s_selected, sizeof(s_selected), "%s", s_target_name);
        lv_label_set_text(g_status_lbl, "Salvo OK");
    } else {
        lv_label_set_text(g_status_lbl, "Falha ao salvar");
    }
    refresh_list();
}

static void load_cb(lv_event_t *e)
{
    (void)e;
    esp_err_t err = profile_manager_load(s_selected);
    lv_label_set_text(g_status_lbl, err == ESP_OK ? "Carregado OK" : "Falha ao carregar");
}

static void del_cb(lv_event_t *e)
{
    (void)e;
    esp_err_t err = profile_manager_delete(s_selected);
    lv_label_set_text(g_status_lbl, err == ESP_OK ? "Excluido" : "Falha ao excluir");
    s_sel_idx = 0;
    refresh_list();
}

static void rename_cb(lv_event_t *e)
{
    (void)e;
    if (strcmp(s_selected, s_target_name) == 0) {
        lv_label_set_text(g_status_lbl, "Nome igual ao atual");
        return;
    }
    esp_err_t err = profile_manager_rename(s_selected, s_target_name);
    if (err == ESP_OK) {
        snprintf(s_selected, sizeof(s_selected), "%s", s_target_name);
        lv_label_set_text(g_status_lbl, "Renomeado OK");
    } else {
        lv_label_set_text(g_status_lbl, "Falha ao renomear");
    }
    refresh_list();
}

static void on_name_edited(const char *name, void *user_data)
{
    (void)user_data;
    snprintf(s_target_name, sizeof(s_target_name), "%s", name);
    refresh_list();
    lv_label_set_text(g_status_lbl, "Nome atualizado");
}

static void edit_cb(lv_event_t *e)
{
    (void)e;
    if (!g_screen_root) return;
    ui_profile_name_editor_show(g_screen_root, s_target_name, on_name_edited, NULL);
}

static void up_cb(lv_event_t *e)
{
    (void)e;
    if (s_sel_idx > 0) s_sel_idx--;
    refresh_list();
}

static void down_cb(lv_event_t *e)
{
    (void)e;
    s_sel_idx++;
    refresh_list();
}

static void back_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_manager_show(UI_SCREEN_CONFIG_HUB);
}

void ui_screen_profiles_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;
    g_screen_root = parent;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Perfis de Config");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_pos(title, 160, 6);

    g_list_lbl = lv_label_create(parent);
    lv_obj_set_size(g_list_lbl, 200, 140);
    lv_obj_set_pos(g_list_lbl, 20, 36);
    lv_obj_set_style_text_font(g_list_lbl, UI_FONT_NORMAL, 0);

    g_target_lbl = lv_label_create(parent);
    lv_obj_set_width(g_target_lbl, 200);
    lv_obj_set_pos(g_target_lbl, 20, 178);
    lv_obj_set_style_text_font(g_target_lbl, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(g_target_lbl, UI_COLOR_TEXT_DIM, 0);

    g_status_lbl = lv_label_create(parent);
    lv_obj_set_width(g_status_lbl, 440);
    lv_obj_set_pos(g_status_lbl, 20, 200);
    lv_obj_set_style_text_color(g_status_lbl, UI_COLOR_TEXT_DIM, 0);

    static const char *btn_txt[] = {
        "Salvar", "Carregar", "Renomear", "Excluir",
        "Editar", "Up", "Down", "Voltar"
    };
    static void (*cbs[])(lv_event_t *) = {
        save_cb, load_cb, rename_cb, del_cb,
        edit_cb, up_cb, down_cb, back_cb
    };
    for (int i = 0; i < 8; i++) {
        lv_obj_t *b = lv_btn_create(parent);
        lv_obj_set_size(b, 82, 26);
        lv_obj_set_pos(b, 240 + (i % 3) * 88, 36 + (i / 3) * 34);
        lv_obj_add_event_cb(b, cbs[i], LV_EVENT_CLICKED, NULL);
        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, btn_txt[i]);
        lv_obj_center(l);
    }

    snprintf(s_selected, sizeof(s_selected), "default");
    snprintf(s_target_name, sizeof(s_target_name), "reef");
    s_sel_idx = 0;
    refresh_list();
}

void ui_screen_profiles_update(ui_root_vm_t *vm)
{
    (void)vm;
}
