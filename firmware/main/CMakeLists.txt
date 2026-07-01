set(MAIN_SRCS
    # Core
    "../core/circuit_breaker.c"
    "../core/event_bus.c"
    "../core/global_state.c"
    "../core/safety_controller.c"

    # Drivers
    "../drivers/driver_acs712.c"
    "../drivers/driver_ad_keypad.c"
    "../drivers/driver_ad_keypad_lvgl.c"
    "../drivers/driver_buzzer_led.c"
    "../drivers/driver_ds18b20.c"
    "../drivers/driver_ds3231.c"
    "../drivers/driver_ili9488.c"
    "../drivers/driver_mcp23017.c"
    "../drivers/driver_mcp3208.c"
    "../drivers/driver_ph_sensor.c"
    "../drivers/driver_pzem.c"
    "../drivers/driver_relay.c"
    "../drivers/driver_xpt2046.c"
    "../drivers/relay_abstraction.c"

    # FSM
    "../fsm/ato_fsm.c"
    "../fsm/feed_fsm.c"
    "../fsm/restart_fsm.c"
    "../fsm/thermal_fsm.c"

    # HAL
    "../hal/hal_bus.c"
    "../hal/hal_spi.c"

    # Security
    "../security/sec_policy.c"

    # Services
    "../services/alert_manager.c"
    "../services/alm_catalog.c"
    "../services/alm_monitor.c"
    "../services/maintenance_mode.c"
    "../services/ato_service.c"
    "../services/audit_log.c"
    "../services/auth_recovery_sd.c"
    "../services/bom_ctl.c"
    "../services/cdn_energy.c"
    "../services/command_dispatcher.c"
    "../services/command_validator.c"
    "../services/conf_ctl.c"
    "../services/config_manager.c"
    "../services/config_export.c"
    "../services/config_root.c"
    "../services/config_staging.c"
    "../services/data_ctl.c"
    "../services/disp_ctl.c"
    "../services/electric_fsm.c"
    "../services/electric_log.c"
    "../services/electric_service.c"
    "../services/feed_snapshot.c"
    "../services/health_matrix.c"
    "../services/log_ctl.c"
    "../services/log_manager.c"
    "../services/pending_actions.c"
    "../services/plug_manager.c"
    "../services/plug_preset_catalog.c"
    "../services/profile_manager.c"
    "../services/relay_safety_service.c"
    "../services/reset_handler.c"
    "../services/safeoff_alm_map.c"
    "../services/safeoff_record.c"
    "../services/safe_state_ack.c"
    "../services/safety_gate.c"
    "../services/self_test.c"
    "../services/sensor_ctl.c"
    "../services/storage_atomic.c"
    "../services/storage_facade.c"
    "../services/storage_manager.c"
    "../services/storage_sd.c"
    "../services/storage_sd_schedule.c"
    "../services/task_manager.c"
    "../services/temp_filter.c"
    "../services/thermal_service.c"
    "../services/time_manager.c"
    "../services/util_ctl.c"
    "../services/watchdog_guard.c"
    "../services/wdt_advanced.c"
    "../services/wdt_stats.c"
    "../services/web_ctl.c"
    "../services/wifi_ctl.c"

    # Web
    "../web/api_auth.c"
    "../web/api_rate_limit.c"
    "../web/api_rest.c"

    # HMI
    "ui/hmi/ui_app.c"
    "ui/hmi/ui_keypad_bridge.c"
    "ui/hmi/ui_events.c"
    "ui/hmi/ui_lvgl_mutex.c"
    "ui/hmi/ui_preset_picker.c"
    "ui/hmi/ui_profile_name_editor.c"
    "ui/hmi/ui_screen_manager.c"
    "ui/hmi/ui_state_machine.c"
    "ui/hmi/ui_theme.c"
    "ui/hmi/ui_view_model.c"
    "ui/hmi/components/ui_alert_row.c"
    "ui/hmi/components/ui_bar_chart.c"
    "ui/hmi/components/ui_confirm_dialog.c"
    "ui/hmi/components/ui_critical_overlay.c"
    "ui/hmi/components/ui_device_row.c"
    "ui/hmi/components/ui_focus.c"
    "ui/hmi/components/ui_footer.c"
    "ui/hmi/components/ui_inline_hint.c"
    "ui/hmi/components/ui_menu_tile.c"
    "ui/hmi/components/ui_metric_card.c"
    "ui/hmi/components/ui_overlay_cause.c"
    "ui/hmi/components/ui_page_indicator.c"
    "ui/hmi/components/ui_status_badge.c"
    "ui/hmi/components/ui_topbar.c"
    "ui/hmi/screens/ui_screen_alerts.c"
    "ui/hmi/screens/ui_screen_ato.c"
    "ui/hmi/screens/ui_screen_config_hub.c"
    "ui/hmi/screens/ui_screen_carousel_settings.c"
    "ui/hmi/screens/ui_screen_config_temperature.c"
    "ui/hmi/screens/ui_screen_dashboard.c"
    "ui/hmi/screens/ui_screen_devices_page1.c"
    "ui/hmi/screens/ui_screen_devices_page2.c"
    "ui/hmi/screens/ui_screen_diag_detail.c"
    "ui/hmi/screens/ui_screen_diagnostics.c"
    "ui/hmi/screens/ui_screen_energy.c"
    "ui/hmi/screens/ui_screen_feed_active.c"
    "ui/hmi/screens/ui_screen_logs.c"
    "ui/hmi/screens/ui_screen_main_menu.c"
    "ui/hmi/screens/ui_screen_profiles.c"
    "ui/hmi/screens/ui_screen_system.c"
    "ui/hmi/screens/ui_screen_wizard.c"
    "ui/hmi/screens/ui_screen_calibration.c"

    # Main task modules
    "handler/api_sensors_handler.c"
    "app/app_main.c"
    "handler/log_handler.c"
    "task/task_web.c"
    "task/ui_task.c"
)

set(MAIN_INCLUDE_DIRS
    "."
    ".."
    "../include"
    "../hal"
    "../drivers"
    "../core"
    "../services"
    "../fsm"
    "../web"
    "../security"
    "app"
    "task"
    "handler"
    "ui/hmi"
    "ui/hmi/components"
    "ui/hmi/screens"
)

idf_component_register(
    SRCS ${MAIN_SRCS}
    INCLUDE_DIRS ${MAIN_INCLUDE_DIRS}
    REQUIRES
        driver
        esp_adc
        esp_timer
        esp_event
        esp_netif
        esp_wifi
        esp_http_server
        nvs_flash
        json
        mbedtls
        fatfs
        sdmmc
        spiffs
        lvgl__lvgl
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror -Wno-error=format-truncation)
