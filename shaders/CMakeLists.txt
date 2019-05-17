cmake_minimum_required(VERSION 2.8.11)

file(GLOB SHADERS_SOURCE *.*)

add_custom_target(shaders SOURCES ${SHADERS_SOURCE})
