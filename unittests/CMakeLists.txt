if(CHECK_FOUND)
  add_executable(csp_tests)
  target_link_libraries(csp_tests PRIVATE csp ${CHECK_LIBRARIES})
  target_sources(csp_tests PRIVATE
    main.c
    queue.c
    buffer.c
    hmac.c
    crc32.c
    rdp_queue.c
  )
endif()

  if(ENABLE_COVERAGE)
    target_compile_options(csp PRIVATE -fprofile-arcs -ftest-coverage)
    target_link_libraries(csp PRIVATE gcov)
  endif()
