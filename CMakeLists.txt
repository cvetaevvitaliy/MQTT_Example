cmake_minimum_required(VERSION 3.16)
project(MQTT_Test C)

set(CMAKE_C_FLAGS_INIT "-std=gnu99  -O2  -Wall -Werror")
set(mosquitto_lib -lmosquitto)
set(SOURCE "main.c")

add_executable(MQTT_Test ${SOURCE} )
target_link_libraries(MQTT_Test ${mosquitto_lib})