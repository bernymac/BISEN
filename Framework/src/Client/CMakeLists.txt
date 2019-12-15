find_package(OpenSSL REQUIRED)

set(CMAKE_C_FLAGS "-pthread -g -O0 -fPIC -Wno-unused-variable -Wall -Wno-unused-function -fstack-protector-all")
set(CMAKE_CXX_FLAGS "-pthread -g -O0 -fPIC -Wno-unused-variable -Wall -Wno-unused-function -fstack-protector-all")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

link_directories(${MBEDTLS_DIR}/build/library)

add_executable(Client ${CLIENT_DIR}/Client.cpp ${CLIENT_DIR}/bisen_tests.cpp ${CLIENT_DIR}/util.cpp ${CLIENT_DIR}/rbisen/ClientCrypt.c ${CLIENT_DIR}/rbisen/Utils.c ${CLIENT_DIR}/rbisen/SseClient.cpp ${CLIENT_DIR}/rbisen/EnglishAnalyzer.cpp ${CLIENT_DIR}/rbisen/PorterStemmer.c ${CLIENT_DIR}/rbisen/QueryParser.cpp)
target_include_directories(Client PRIVATE ${CLIENT_DIR} ${FRAMEWORK_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${CLIENT_DIR}/rbisen)
target_link_libraries(Client ${LIB_UNTRUSTED} ${LIB_UNTRUSTED_TLS} ${OPENSSL_CRYPTO_LIBRARY} config)
