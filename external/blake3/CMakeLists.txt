
#blake3
FetchContent_Declare(
        blake3
        GIT_REPOSITORY https://github.com/BLAKE3-team/BLAKE3.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(blake3)

set(BLAKE3_SRC
        ${blake3_SOURCE_DIR}/c/blake3.c
        ${blake3_SOURCE_DIR}/c/blake3_dispatch.c
        ${blake3_SOURCE_DIR}/c/blake3_portable.c
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    include(CheckCCompilerFlag)

    check_c_compiler_flag("-msse2"   HAS_SSE2)
    check_c_compiler_flag("-msse4.1" HAS_SSE41)
    check_c_compiler_flag("-mavx2"   HAS_AVX2)
    check_c_compiler_flag("-mavx512f" HAS_AVX512)

    if(HAS_SSE2)
        list(APPEND BLAKE3_SRC ${blake3_SOURCE_DIR}/c/blake3_sse2.c)
    endif()
    if(HAS_SSE41)
        list(APPEND BLAKE3_SRC ${blake3_SOURCE_DIR}/c/blake3_sse41.c)
    endif()
    if(HAS_AVX2)
        list(APPEND BLAKE3_SRC ${blake3_SOURCE_DIR}/c/blake3_avx2.c)
    endif()
    if(HAS_AVX512)
        list(APPEND BLAKE3_SRC ${blake3_SOURCE_DIR}/c/blake3_avx512.c)
    endif()
endif()

if(HAS_SSE2)
    set_source_files_properties(${blake3_SOURCE_DIR}/c/blake3_sse2.c PROPERTIES COMPILE_FLAGS "-msse2")
endif()
if(HAS_SSE41)
    set_source_files_properties(${blake3_SOURCE_DIR}/c/blake3_sse41.c PROPERTIES COMPILE_FLAGS "-msse4.1")
endif()
if(HAS_AVX2)
    set_source_files_properties(${blake3_SOURCE_DIR}/c/blake3_avx2.c PROPERTIES COMPILE_FLAGS "-mavx2")
endif()
if(HAS_AVX512)
    set_source_files_properties(${blake3_SOURCE_DIR}/c/blake3_avx512.c PROPERTIES COMPILE_FLAGS "-mavx512f -mavx512vl")
endif()

add_library(blake3 STATIC ${BLAKE3_SRC})
set_target_properties(blake3 PROPERTIES LINKER_LANGUAGE C)
target_include_directories(blake3 PUBLIC ${blake3_SOURCE_DIR}/c)
add_library(BLAKE3::blake3 ALIAS blake3)