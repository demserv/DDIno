idf_component_register(
    SRCS
        test_safety.c
        test_thermal.c
        test_electric.c
        test_ato.c
        test_feed.c
        test_alert.c
        test_temp_filter.c
        test_plug_manager.c
    INCLUDE_DIRS
        "${PROJECT_DIR}"
        "${PROJECT_DIR}/include"
        "${PROJECT_DIR}/core"
        "${PROJECT_DIR}/fsm"
        "${PROJECT_DIR}/services"
        "${PROJECT_DIR}/drivers"
        "${PROJECT_DIR}/hal"
    REQUIRES unity driver nvs_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-function -Wno-unused-variable)
