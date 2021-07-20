cmake_minimum_required(VERSION 3.20.4)

if(CONFIG_MY_UART_PERIPHERAL)
include_directories(.)
target_sources(app PRIVATE driver.c)
endif()