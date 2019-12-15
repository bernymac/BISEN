set(CMAKE_C_FLAGS "-pthread -g -O0 -fPIC -Wall")
set(CMAKE_CXX_FLAGS "-pthread -g -O0 -fPIC -Wall")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

link_directories(${MBEDTLS_DIR}/build/library)

if(SERVER_ONLY)
    set(SOURCES ${LIB_UNTRUSTED_DIR}/untrusted_util.cpp ${LIB_UNTRUSTED_DIR}/untrusted_crypto.cpp)
else()
    set(SOURCES ${LIB_UNTRUSTED_DIR}/untrusted_util.cpp ${LIB_UNTRUSTED_DIR}/untrusted_crypto.cpp ${LIB_UNTRUSTED_DIR}/secure_conn.cpp)
endif()


add_library(${LIB_UNTRUSTED} ${SOURCES})

target_include_directories(${LIB_UNTRUSTED} PRIVATE ${FRAMEWORK_INCLUDE_DIR} ${MBEDTLS_DIR}/include)

if(SERVER_ONLY)
    target_link_libraries(${LIB_UNTRUSTED})
else()
    target_link_libraries(${LIB_UNTRUSTED} sodium mbedcrypto mbedtls mbedx509)
endif()
