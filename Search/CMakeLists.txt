# sgx vars
if (NOT DEFINED ENV{SGX_SDK})
    set(SGX_SDK "/opt/intel/sgxsdk")
else()
    set(SGX_SDK $ENV{SGX_SDK})
endif ()

# include paths for the sgx versions of standard libs
set(SGX_INCLUDE_PATHS ${SGX_SDK}/include ${SGX_SDK}/include/libcxx ${SGX_SDK}/include/tlibc)

# flags
set(DEBUG_FLAGS "-g -O0")
if (SIMULATED_MODE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEBUG_FLAGS} -Wno-pointer-arith -c -Wall -fPIC -Wall -m64 -fvisibility=hidden -fpie -fstack-protector")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEBUG_FLAGS} -Wno-pointer-arith -c -Wall -fPIC -Wall -m64 -fvisibility=hidden -fpie -fstack-protector -fno-builtin-function -nostdinc++")
endif ()

set(CMAKE_CXX_STANDARD 11)

# sources
set(UTIL_SRCS ${EXTERN_DIR}/src/util.cpp)

# define target library
add_library(${EXTERN_LIB_NAME}
        ${EXTERN_DIR}/src/rbisen/util/IeeUtils.cpp ${EXTERN_DIR}/src/rbisen/util/vec_int.cpp ${EXTERN_DIR}/src/rbisen/util/vec_token.cpp
        ${EXTERN_DIR}/src/rbisen/QueryEvaluator.cpp ${EXTERN_DIR}/src/rbisen/SseIee.cpp
        ${EXTERN_DIR}/src/trusted.cpp
        ${EXTERN_DIR}/src/untrusted.cpp
        ${UTIL_SRCS})

if (SIMULATED_MODE)
    target_link_libraries(${EXTERN_LIB_NAME} trusted_util)
else()
    target_link_libraries(${EXTERN_LIB_NAME})
endif ()

if (SIMULATED_MODE)
    target_include_directories(${EXTERN_LIB_NAME} PRIVATE ${EXTERN_DIR} ${EXTERN_DIR}/src ${FRAMEWORK_INCLUDE_DIR} ${EXTERN_DIR}/src/rbisen ${EXTERN_DIR}/src/rbisen/util)
else()
    target_include_directories(${EXTERN_LIB_NAME} PRIVATE ${EXTERN_DIR} ${EXTERN_DIR}/src ${FRAMEWORK_INCLUDE_DIR} ${SGX_INCLUDE_PATHS} ${EXTERN_DIR}/src/rbisen ${EXTERN_DIR}/src/rbisen/util)
endif ()
