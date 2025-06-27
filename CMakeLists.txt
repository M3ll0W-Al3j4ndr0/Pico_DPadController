cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

set(PROJECT_NAME "dPadController")

project(${PROJECT_NAME})

pico_sdk_init()

# Make sure TinyUSB can find tusb_config.h
include_directories(${PROJECT_SOURCE_DIR})

add_executable(${PROJECT_NAME}
	main.c
	usb_descriptors.c
	preloaded.h
	)


target_link_libraries(${PROJECT_NAME} 
	pico_stdlib 
	tinyusb_device 
	tinyusb_board
	pico_multicore
	hardware_adc
	pico_unique_id
	)

pico_add_extra_outputs(${PROJECT_NAME})

