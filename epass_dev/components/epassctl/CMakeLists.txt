cmake_minimum_required(VERSION 3.16)
project(cra_epassctl VERSION 1.0.0 LANGUAGES C)

add_executable(epassctl
    src/main.c
    src/ipc_client.c
    ../protocol/src/ipc_v1.c
)

target_include_directories(epassctl PRIVATE
    include
    ../protocol/include
)
target_compile_features(epassctl PRIVATE c_std_11)
target_compile_options(epassctl PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

include(GNUInstallDirs)
install(TARGETS epassctl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CTest)
if(BUILD_TESTING)
    enable_testing()
    add_executable(protocol_layout_test
        tests/protocol_layout_test.c
        ../protocol/src/ipc_v1.c
    )
    target_include_directories(protocol_layout_test PRIVATE ../protocol/include)
    target_compile_features(protocol_layout_test PRIVATE c_std_11)
    add_test(NAME protocol_layout COMMAND protocol_layout_test)

    add_executable(ipc_roundtrip_test
        tests/ipc_roundtrip_test.c
        src/ipc_client.c
    )
    target_include_directories(ipc_roundtrip_test PRIVATE include)
    target_compile_features(ipc_roundtrip_test PRIVATE c_std_11)
    target_link_libraries(ipc_roundtrip_test PRIVATE pthread)
    add_test(NAME ipc_roundtrip COMMAND ipc_roundtrip_test)
endif()
