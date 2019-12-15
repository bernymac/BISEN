# flags
set(c_flags "${sgx_common_flags} -fPIC -Wall -Wno-attributes -DDEBUG -UNDEBUG -UEDEBUG")
set(CMAKE_C_FLAGS ${c_flags})
set(CMAKE_CXX_FLAGS ${c_flags})

add_custom_command(
    OUTPUT ${IEE_COMM_DIR}/Enclave_u.h ${IEE_COMM_DIR}/Enclave_u.c
    COMMAND ${SGX_EDGER8R} --untrusted-dir ${IEE_COMM_DIR} --untrusted ${IEE_DIR}/Enclave.edl --search-path ${IEE_DIR} --search-path ${FRAMEWORK_INCLUDE_DIR} --search-path ${MBEDTLS_SGX_DIR}/lib --search-path ${SGX_SDK}/include
    DEPENDS ${IEE_DIR}/Enclave.edl
)
add_custom_target(edger8r_untrusted ALL DEPENDS ${IEE_COMM_DIR}/Enclave_u.h ${IEE_COMM_DIR}/Enclave_u.c)

link_directories(${SGX_LIBRARY_PATH} ${MBEDTLS_SGX_DIR}/lib ${MBEDTLS_DIR}/build/library)

#find_library(URTS sgx_urts PATHS /usr/lib NO_CMAKE_FIND_ROOT_PATH)
#find_library(UAE_SERVICE sgx_uae_service PATHS /usr/lib NO_CMAKE_FIND_ROOT_PATH)

add_executable(IeeComm
        ${IEE_COMM_DIR}/App.cpp ${IEE_COMM_DIR}/Enclave_u.c
        ${IEE_COMM_DIR}/ocall/ocall.cpp
        ${IEE_COMM_DIR}/sgx_handler/sgx_handler.cpp
        ${IEE_COMM_DIR}/threading/thread_pool.cpp
        ${IEE_COMM_DIR}/mbedtls/mbedtls_error.c ${IEE_COMM_DIR}/mbedtls/mbedtls_net.c)

add_dependencies(IeeComm edger8r_untrusted)

target_include_directories(IeeComm PRIVATE
        ${FRAMEWORK_INCLUDE_DIR}
        ${EXTERN_INCLUDE_DIR}
        ${IEE_COMM_DIR}
        ${IEE_COMM_DIR}/mbedtls
        ${IEE_COMM_DIR}/ocall
        ${IEE_COMM_DIR}/sgx_handler
        ${IEE_COMM_DIR}/threading
        ${SGX_SDK}/include
        ${MBEDTLS_SGX_DIR}/include)
target_link_libraries(IeeComm ${LIB_UNTRUSTED} sgx_urts pthread sgx_uae_service sgx_uprotected_fs mbedtls_SGX_u)
