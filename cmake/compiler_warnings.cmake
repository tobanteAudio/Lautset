add_library(lautset_compiler_warnings INTERFACE)
add_library(lautset::compiler_warnings ALIAS lautset_compiler_warnings)

if((CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") OR (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    target_compile_options(lautset_compiler_warnings INTERFACE /W4 $<$<BOOL:${LAUTSET_ENABLE_WERROR}>:/WX>)
else ()
    target_compile_options(lautset_compiler_warnings INTERFACE -Wall -Wextra -Wpedantic $<$<BOOL:${LAUTSET_ENABLE_WERROR}>:-Werror>)
endif ()