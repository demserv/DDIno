cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS "test")
set(TEST_COMPONENTS "test")
set(TEST_EXCLUDE_COMPONENTS "main")

project(monitor_aquario_inteligente_fw)
