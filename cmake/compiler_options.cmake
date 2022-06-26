add_library(lautset_compiler_options INTERFACE)
add_library(lautset::compiler_options ALIAS lautset_compiler_options)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(lautset_compiler_options INTERFACE "/permissive-")
endif()

if(LAUTSET_ENABLE_ASAN AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lautset_compiler_options INTERFACE -fsanitize=address -fsanitize-address-use-after-scope -O1 -g -fno-omit-frame-pointer)
    target_link_libraries(lautset_compiler_options INTERFACE -fsanitize=address -fsanitize-address-use-after-scope -O1 -g -fno-omit-frame-pointer)
endif()

if(LAUTSET_ENABLE_UBSAN AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lautset_compiler_options INTERFACE -fsanitize=undefined -O1 -g -fno-omit-frame-pointer)
    target_link_libraries(lautset_compiler_options INTERFACE -fsanitize=undefined -O1 -g -fno-omit-frame-pointer)
endif()

if(LAUTSET_ENABLE_MSAN AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lautset_compiler_options INTERFACE -fsanitize=memory -O1 -g -fno-omit-frame-pointer)
    target_link_libraries(lautset_compiler_options INTERFACE -fsanitize=memory -O1 -g -fno-omit-frame-pointer)
endif()

if(LAUTSET_ENABLE_TSAN AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lautset_compiler_options INTERFACE -fsanitize=thread -O1 -g -fno-omit-frame-pointer)
    target_link_libraries(lautset_compiler_options INTERFACE -fsanitize=thread -O1 -g -fno-omit-frame-pointer)
endif()
