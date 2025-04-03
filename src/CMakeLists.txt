set(gcc_like_c "$<COMPILE_LANG_AND_ID:C,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_like_c "$<COMPILE_LANG_AND_ID:C,MSVC>")

if (BuildType MATCHES "Release")
    set(EXTRA_COMPILE_OPTIONS $<${gcc_like_c}:-O3>)
    set(enable_logging 0)
elseif(BuildType MATCHES "Debug")
    set(EXTRA_COMPILE_OPTIONS $<${gcc_like_c}:-O0 -ggdb>)
    set(enable_logging 1)
endif()

add_library(sealloc)

target_sources(sealloc
    PRIVATE malloc_api.c sealloc.c platform.c chunk.c bin.c internal_allocator.c random.c run.c container_ll.c arena.c size_class.c
)
target_include_directories(sealloc
    PRIVATE "src/"
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_compile_options(sealloc
    PRIVATE $<${gcc_like_c}:-Wall -Werror -Wextra -pedantic -Wformat=0> ${EXTRA_COMPILE_OPTIONS}
)
target_compile_definitions(sealloc PRIVATE "$<$<BOOL:${enable_logging}>:LOGGING>")

include(GenerateExportHeader)
set_property(
    TARGET sealloc
    PROPERTY C_VISIBILITY_PRESET "hidden"
)
set_property(
    TARGET sealloc
    PROPERTY VISIBILITY_INLINES_HIDDEN TRUE
)
generate_export_header(sealloc
    EXPORT_FILE_NAME "export/sealloc/sealloc_export.h")
target_include_directories(
    sealloc
    PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src/export/>
)
