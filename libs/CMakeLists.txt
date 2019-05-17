cmake_minimum_required(VERSION 2.8.11)

file(GLOB LIBS_SOURCE *.*)

add_custom_target(libs SOURCES ${LIBS_SOURCE})
