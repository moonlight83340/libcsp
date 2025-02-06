add_executable(simple-udp-server ${CSP_SAMPLES_EXCLUDE} src/main.c)
target_include_directories(simple-udp-server PRIVATE ${csp_inc})
target_link_libraries(simple-udp-server PRIVATE csp pthread)
