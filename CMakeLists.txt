cmake_minimum_required(VERSION 3.5)
project(bisen)

if(NOT DEFINED SERVER_ONLY)
    set(SERVER_ONLY 0)
endif()

if(NO_CLIENT)
    set(MAKE_CLIENT 0)
else()
    set(MAKE_CLIENT 1)
endif()

set(FRAMEWORK_NAME Framework)

# base paths
set(MBEDTLS_SGX_DIR ~/lib/mbedtls-SGX/build/mbedtls_SGX-2.6.0)
set(MBEDTLS_DIR ~/lib/mbedtls)

set(FRAMEWORK_DIR ${CMAKE_SOURCE_DIR}/${FRAMEWORK_NAME})
set(EXTERN_DIR ${CMAKE_SOURCE_DIR}/Search)
set(EXTERN_LIB_NAME Search)

########################################################################################################################
########################################### PROBABLY NOT NEEDED TO EDIT BELOW ##########################################
########################################################################################################################

# paths for binaries
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

# include dirs
set(FRAMEWORK_INCLUDE_DIR ${FRAMEWORK_DIR}/include)
set(EXTERN_INCLUDE_DIR ${EXTERN_DIR}/include) # not used

if(SERVER_ONLY)
    add_subdirectory(${FRAMEWORK_DIR})
else()
    add_subdirectory(${EXTERN_DIR})
    add_subdirectory(${FRAMEWORK_DIR})
endif()
