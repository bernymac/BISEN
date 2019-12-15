#TODO need to remove /usr/local/bin/sgxsdk/lib64/libsgx_urts.so & /usr/local/bin/sgxsdk/lib64/libsgx_uae_service.so

set(ENCLAVE_KEY_FILE ${IEE_DIR}/sgx_config/Enclave_private.pem)
set(ENCLAVE_CONFIG_FILE ${IEE_DIR}/sgx_config/Enclave.config.xml)
set(ENCLAVE_LINKER_FILE ${IEE_DIR}/sgx_config/Enclave_debug.lds)

# flags
set(CMAKE_C_FLAGS ${sgx_c_flags})
set(CMAKE_CXX_FLAGS ${sgx_cpp_flags})

add_custom_command(
    OUTPUT ${IEE_DIR}/Enclave_t.h ${IEE_DIR}/Enclave_t.c
    COMMAND ${SGX_EDGER8R} --trusted-dir ${IEE_DIR} --trusted ${IEE_DIR}/Enclave.edl --search-path ${IEE_DIR} --search-path ${FRAMEWORK_INCLUDE_DIR} --search-path ${MBEDTLS_SGX_DIR}/lib --search-path ${SGX_SDK}/include
    DEPENDS ${IEE_DIR}/Enclave.edl
)
add_custom_target(edger8r_trusted ALL DEPENDS ${IEE_DIR}/Enclave_t.h ${IEE_DIR}/Enclave_t.c)

link_directories(${SGX_LIBRARY_PATH} ${MBEDTLS_SGX_DIR}/lib)

add_executable(${ENCLAVE_NAME}
        ${IEE_DIR}/Enclave_t.c ${IEE_DIR}/Enclave.cpp ${IEE_DIR}/thread_handler.cpp
        ${IEE_DIR}/mbedtls/ecalls.cpp ${IEE_DIR}/mbedtls/pprint.c ${IEE_DIR}/mbedtls/ssl_conn_hdlr.cpp)
add_dependencies(${ENCLAVE_NAME} edger8r_trusted)

# includes and linking
target_include_directories(${ENCLAVE_NAME} PRIVATE ${FRAMEWORK_INCLUDE_DIR} /usr/local/include ${EXTERN_INCLUDE_DIR} ${MBEDTLS_SGX_DIR}/include ${IEE_DIR} ${IEE_DIR}/ocall_wrappers ${IEE_DIR}/threading ${SGX_INCLUDE_PATHS})

# To generate a proper enclave, it is recommended to follow below guideline to link the trusted libraries:
#    1. Link sgx_trts with the `--whole-archive' and `--no-whole-archive' options,
#       so that the whole content of trts is included in the enclave.
#    2. For other libraries, you just need to pull the required symbols.
#       Use `--start-group' and `--end-group' to link these libraries.
# Do NOT move the libraries linked with `--start-group' and `--end-group' within `--whole-archive' and `--no-whole-archive' options.
# Otherwise, you may get some undesirable errors.
target_link_libraries(${ENCLAVE_NAME} -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles
    -Wl,--whole-archive sgx_trts -Wl,--no-whole-archive
    -Wl,--start-group sgx_tstdc sgx_tcxx sgx_tcrypto sgx_tservice sgx_tprotected_fs mbedtls_SGX_t ${LIB_TRUSTED} ${EXTERN_LIB_NAME} -Wl,--end-group
    -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined
    -Wl,-pie,-eenclave_entry -Wl,--export-dynamic
    -Wl,--defsym,__ImageBase=0
    -Wl,--version-script=${ENCLAVE_LINKER_FILE}
    ${EXTERN_LIB_NAME}
)

add_custom_command(
    OUTPUT enclave
    COMMAND ${SGX_ENCLAVE_SIGNER} sign -key ${ENCLAVE_KEY_FILE} -enclave ${EXECUTABLE_OUTPUT_PATH}/${ENCLAVE_NAME} -out ${EXECUTABLE_OUTPUT_PATH}/${SIGNED_ENCLAVE_NAME} -config ${ENCLAVE_CONFIG_FILE}
    DEPENDS ${ENCLAVE_NAME} ${EXTERN_LIB_NAME}
)

add_custom_target(signer ALL DEPENDS enclave)

