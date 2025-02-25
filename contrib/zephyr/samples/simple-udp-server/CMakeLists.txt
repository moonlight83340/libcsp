# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(csp_udp_server)

target_sources(app PRIVATE main.c csp_server_zephyr.c)
target_link_libraries(app PRIVATE csp)
