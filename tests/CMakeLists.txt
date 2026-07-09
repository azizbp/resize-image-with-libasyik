# Reuses the Catch2 (v2) already bundled by libasyik to avoid a second copy
add_executable(test_resize test_resize.cpp)
target_link_libraries(test_resize PRIVATE resize_core Catch2::Catch2)

add_test(NAME test_resize COMMAND test_resize)
