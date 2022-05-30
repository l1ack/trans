cmake_minimum_required(VERSION 3.17)
if(NOT WIN32)
if(LINUX)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
        set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif(LINUX)

set(TARGET_NAME_xmit_demo_client_lib xmit-demo-client-lib)
add_library(${TARGET_NAME_xmit_demo_client_lib} STATIC
  client.cc
  client.h
  ${PROJECT_SOURCE_DIR}/src/common/platforms.h
  )

target_compile_features(${TARGET_NAME_xmit_demo_client_lib} PUBLIC cxx_std_17)
target_include_directories(${TARGET_NAME_xmit_demo_client_lib} PUBLIC
  ${PROJECT_SOURCE_DIR}/src/
  ${PROJECT_SOURCE_DIR}/src/common
  ${PROJECT_SOURCE_DIR}/3rdparty/protobuf/src
)

#glog
#libprotobuf
set(client_lib_LINKS xmit-api)
if(LINUX)
  list(APPEND client_lib_LINKS pthread)
elseif(WIN32)
endif()

if(ANDROID)
  find_library(log_lib log)
endif()

target_link_libraries(${TARGET_NAME_xmit_demo_client_lib} PUBLIC
  ${client_lib_LINKS}
  ${log_lib}
)

set(client_LINKS ${TARGET_NAME_xmit_demo_client_lib})
if(LINUX)
    list(APPEND client_LINKS debug asan)
endif()

set(DEMO_CLIENT_NAME xmit-demo-client)
set(DEMO_CLIENT_NAME_LOOPBACK xmit-demo-client-loopback)
add_executable(${DEMO_CLIENT_NAME} main.cc)
target_link_libraries(${DEMO_CLIENT_NAME} PUBLIC ${client_LINKS})
add_executable(${DEMO_CLIENT_NAME_LOOPBACK} loopback_main.cc)
target_link_libraries(${DEMO_CLIENT_NAME_LOOPBACK} PUBLIC ${client_LINKS})

if(LINUX OR ANDROID)
  add_custom_command(TARGET ${DEMO_CLIENT_NAME} POST_BUILD
                    COMMAND rsync -a $<TARGET_FILE:${DEMO_CLIENT_NAME}> ${PROJECT_SOURCE_DIR}/demo/client)
  add_custom_command(TARGET ${DEMO_CLIENT_NAME_LOOPBACK} POST_BUILD
                    COMMAND rsync -a $<TARGET_FILE:${DEMO_CLIENT_NAME_LOOPBACK}> ${PROJECT_SOURCE_DIR}/demo/loopback-client)
endif()
define_binary_directory(${TARGET_NAME_xmit_demo_client_lib})
define_binary_directory(${DEMO_CLIENT_NAME})
define_binary_directory(${DEMO_CLIENT_NAME_LOOPBACK})
endif()
