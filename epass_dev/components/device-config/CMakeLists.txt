cmake_minimum_required(VERSION 3.16)
project(cra_device_config VERSION 1.0.0 LANGUAGES C)

add_library(cra_config_core STATIC src/uenv.c src/config_state.c)
target_include_directories(cra_config_core PUBLIC include)
target_compile_features(cra_config_core PUBLIC c_std_11)
target_compile_options(cra_config_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

add_executable(cra_device_config src/main.c)
target_link_libraries(cra_device_config PRIVATE cra_config_core)
target_compile_features(cra_device_config PRIVATE c_std_11)
target_compile_options(cra_device_config PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

include(GNUInstallDirs)
install(TARGETS cra_device_config RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CTest)
if(BUILD_TESTING)
    enable_testing()
    add_executable(device_config_test tests/config_test.c)
    target_link_libraries(device_config_test PRIVATE cra_config_core)
    add_test(NAME device_config COMMAND device_config_test)
endif()
