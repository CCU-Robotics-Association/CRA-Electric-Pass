cmake_minimum_required(VERSION 3.16)
project(cra_usb_responder VERSION 1.0.0 LANGUAGES C)

add_library(cra_usb_protocol STATIC src/protocol.c)
target_include_directories(cra_usb_protocol PUBLIC include)
target_compile_features(cra_usb_protocol PUBLIC c_std_11)
target_compile_options(cra_usb_protocol PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

add_library(cra_usb_service STATIC src/service.c src/command.c)
target_include_directories(cra_usb_service PUBLIC include)
target_compile_features(cra_usb_service PUBLIC c_std_11)
target_compile_options(cra_usb_service PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
target_link_libraries(cra_usb_service PUBLIC cra_usb_protocol)

add_executable(usb_responder src/main.c src/functionfs.c)
target_include_directories(usb_responder PRIVATE include)
target_compile_features(usb_responder PRIVATE c_std_11)
target_compile_options(usb_responder PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
target_link_libraries(usb_responder PRIVATE cra_usb_service)

include(GNUInstallDirs)
install(TARGETS usb_responder RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CTest)
if(BUILD_TESTING)
    enable_testing()
    add_executable(usb_protocol_test tests/protocol_test.c)
    target_link_libraries(usb_protocol_test PRIVATE cra_usb_protocol)
    add_test(NAME usb_protocol COMMAND usb_protocol_test)

    add_executable(usb_service_test tests/service_test.c)
    target_link_libraries(usb_service_test PRIVATE cra_usb_service)
    add_test(NAME usb_service COMMAND usb_service_test)
endif()
