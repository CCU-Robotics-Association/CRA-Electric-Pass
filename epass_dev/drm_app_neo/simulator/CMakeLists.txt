cmake_minimum_required(VERSION 3.16)

project(epass_ui_simulator LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# Keep the desktop target independent from the ARM application build.  LVGL's
# CMake project reads this configuration file only for this build directory.
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(LV_BUILD_SET_CONFIG_OPTS OFF CACHE BOOL "" FORCE)
set(LV_BUILD_LVGL_H_SIMPLE_INCLUDE OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
find_package(PNG REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory("${REPO_ROOT}/lvgl" "${CMAKE_CURRENT_BINARY_DIR}/lvgl")

file(GLOB UI_SOURCES CONFIGURE_DEPENDS
    "${REPO_ROOT}/eez_design/src/ui/*.c"
)

add_executable(epass_ui_simulator
    main.c
    sim_actions.c
    sim_vars.c
    "${REPO_ROOT}/src/ui/cat_pet.c"
    "${REPO_ROOT}/src/ui/theme.c"
    ${UI_SOURCES}
)

target_include_directories(epass_ui_simulator PRIVATE
    "${REPO_ROOT}"
    "${REPO_ROOT}/src"
    "${REPO_ROOT}/eez_design/src/ui"
)

target_link_libraries(epass_ui_simulator PRIVATE
    lvgl
    PkgConfig::SDL2
    PNG::PNG
    Threads::Threads
    m
)

target_compile_options(epass_ui_simulator PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
