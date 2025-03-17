add_library(sealloc bin.c internal_allocator.c random.c run.c container_ll.c)

set(gcc_like_c "$<COMPILE_LANG_AND_ID:C,ARMClang,AppleClang,Clang,GNU,LCC>")
if (BuildType MATCHES "Release")
    set(EXTRA_COMPILE_OPTIONS $<${gcc_like_c}:-03>)
    set(enable_logging 0)
elseif(BuildType MATCHES "Debug")
    set(EXTRA_COMPILE_OPTIONS $<${gcc_like_c}:-O0 -ggdb>)
    set(enable_logging 1)
endif()

target_include_directories(sealloc PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_options(sealloc PRIVATE
    $<${gcc_like_c}:-Wall -Werror -pedantic -Wformat=0>
    ${EXTRA_COMPILE_OPTIONS}
)
target_compile_definitions(sealloc PRIVATE "$<$<BOOL:${enable_logging}>:LOGGING>")
