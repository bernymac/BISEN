# flags
set(COMMON_FLAGS "-pthread -g -O0 -fPIC -Wall -Wno-unused-function -Wno-unused-variable -Wno-sign-compare")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

######################################################### SGX #########################################################
# define common sgx vars
if(NOT DEFINED ENV{SGX_SDK})
    set(SGX_SDK /opt/intel/sgxsdk)
else()
    set(SGX_SDK $ENV{SGX_SDK})
endif()

set(SGX_LIBRARY_PATH ${SGX_SDK}/lib64)
set(SGX_EDGER8R ${SGX_SDK}/bin/x64/sgx_edger8r)
set(SGX_ENCLAVE_SIGNER ${SGX_SDK}/bin/x64/sgx_sign)

# filenames for enclave
set(ENCLAVE_NAME enclave.so)
set(SIGNED_ENCLAVE_NAME enclave.signed.so)
add_definitions(-DSIGNED_ENCLAVE_NAME="${SIGNED_ENCLAVE_NAME}")
add_definitions(-DENCLAVE_TOKEN_NAME="enclave.token")

# sgx flags
set(SGX_INCLUDE_PATHS ${SGX_SDK}/include ${SGX_SDK}/include/libcxx ${SGX_SDK}/include/tlibc) # include paths for the sgx versions of standard libs
set(sgx_common_flags "-m64 -O0 -g")
set(sgx_c_flags "${sgx_common_flags} -nostdinc -fvisibility=hidden -fpie -fstack-protector -ffunction-sections -fdata-sections -fstack-protector-strong")
set(sgx_cpp_flags "${sgx_c_flags} -nostdinc++")

######################################################### SGX #########################################################

###
#also, if using, needs to know IEE_DIR IEE_COMM_DIR, which are currently below
#add_custom_command(
#    OUTPUT ${IEE_DIR}/Enclave_t.h ${IEE_DIR}/Enclave_t.c ${IEE_COMM_DIR}/Enclave_u.h ${IEE_COMM_DIR}/Enclave_u.c
#    COMMAND ${SGX_EDGER8R} --trusted-dir ${IEE_DIR} --untrusted-dir ${IEE_COMM_DIR} --search-path ${IEE_DIR} --search-path ${FRAMEWORK_INCLUDE_DIR} --search-path $(SGX_SDK)/include ${IEE_DIR}/Enclave.edl
#    DEPENDS ${IEE_DIR}/Enclave.edl
#)

#add_custom_target(edger8r ALL DEPENDS ${IEE_DIR}/Enclave_t.h ${IEE_DIR}/Enclave_t.c ${IEE_COMM_DIR}/Enclave_u.h ${IEE_COMM_DIR}/Enclave_u.c)

###

# directories for modules
set(LIB_UNTRUSTED_DIR ${FRAMEWORK_DIR}/src/LibUntrusted)
set(LIB_TRUSTED_DIR ${FRAMEWORK_DIR}/src/LibTrusted)
set(STORAGE_DIR ${FRAMEWORK_DIR}/src/StorageService)
set(CLIENT_DIR ${FRAMEWORK_DIR}/src/Client)
set(IEE_COMM_DIR ${FRAMEWORK_DIR}/src/IeeComm)
set(IEE_DIR ${FRAMEWORK_DIR}/src/Iee)

# lib untrusted
set(LIB_UNTRUSTED untrusted_util)
add_subdirectory(${LIB_UNTRUSTED_DIR})

if(SERVER_ONLY)
    # storage module
    add_subdirectory(${STORAGE_DIR})
else()
    # lib trusted
    set(LIB_TRUSTED trusted_util)
    add_subdirectory(${LIB_TRUSTED_DIR})

    # storage module
    add_subdirectory(${STORAGE_DIR})

    # client module
    if(MAKE_CLIENT)
        add_subdirectory(${CLIENT_DIR})
    endif()

    # iee_comm module
    add_subdirectory(${IEE_COMM_DIR})

    # iee module
    add_subdirectory(${IEE_DIR})
endif()
