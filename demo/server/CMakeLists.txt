cmake_minimum_required(VERSION 3.17)
if(NOT WIN32)
if(LINUX)
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
  set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif(LINUX)

set(TARGET_NAME_xmit_demo_server_lib xmit-demo-server-lib)
add_library(${TARGET_NAME_xmit_demo_server_lib} STATIC
  server.cc
  server.h
)

target_compile_features(${TARGET_NAME_xmit_demo_server_lib} PUBLIC cxx_std_17)
target_include_directories(${TARGET_NAME_xmit_demo_server_lib} PUBLIC
  ${PROJECT_SOURCE_DIR}/src/
  ${PROJECT_SOURCE_DIR}/3rdparty/protobuf/src
)

#glog
# libprotobuf
set(server_lib_LINKS xmit-api)
if(LINUX)
  list(APPEND server_lib_LINKS pthread)
elseif(WIN32)
endif()

if(ANDROID)
  find_library(log_lib log)
endif()

target_link_libraries(${TARGET_NAME_xmit_demo_server_lib} PUBLIC
  ${server_lib_LINKS}
  ${log_lib}
)

set(server_LINKS ${TARGET_NAME_xmit_demo_server_lib})
if(LINUX)
    list(APPEND server_LINKS debug asan)
endif()

add_executable(xmit-demo-server main.cc)
target_link_libraries(xmit-demo-server PUBLIC ${server_LINKS})
add_executable(xmit-demo-server-loopback loopback_main.cc)
target_link_libraries(xmit-demo-server-loopback PUBLIC ${server_LINKS})
                         
if(LINUX OR ANDROID)
  add_custom_command(TARGET xmit-demo-server POST_BUILD
                    COMMAND rsync -a $<TARGET_FILE:xmit-demo-server> ${PROJECT_SOURCE_DIR}/demo/server)
  add_custom_command(TARGET xmit-demo-server-loopback POST_BUILD
                    COMMAND rsync -a $<TARGET_FILE:xmit-demo-server-loopback> ${PROJECT_SOURCE_DIR}/demo/loopback-server)
endif()
              define_binary_directory(${TARGET_NAME_xmit_demo_server_lib})
endif()
        