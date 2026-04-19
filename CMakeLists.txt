cmake_minimum_required(VERSION 3.10)
project(RingBufferApp C)

set(CMAKE_C_STANDARD 99)
set(SOURCES main.c ring_buffer.c)

add_executable(ring_buffer_app ${SOURCES})

enable_testing()
add_executable(ring_buffer_tests ring_buffer_tests.c ring_buffer.c)
add_test(NAME ring_buffer_tests COMMAND ring_buffer_tests)

add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --test-dir ${CMAKE_BINARY_DIR}
    DEPENDS ring_buffer_tests
)
