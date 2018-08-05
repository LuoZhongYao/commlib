project (commlib)
cmake_minimum_required (VERSION 3.8)

set (CMAKE_C_STANDARD 11)
set (CMAKE_C_STANDARD_REQUIRED TRUE)

include_directories(include)
set(CMAKE_C_FLAGS_DEBUG "-g" )
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "(arm)|(ARM)")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon")
endif()

if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color")
    endif()
elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics -Wno-pointer-bool-conversion -Wno-parentheses-equality")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics -Wno-pointer-bool-conversion -Wno-parentheses-equality")
endif()

add_library(commlib 
    src/urwlib.c
    src/umod.c
    src/getpname.c
    src/kfifo.c
    src/btsnoop.c)
target_link_libraries(commlib)


set(CMAKE_BUILD_WITH_INSTALL_RPATH true)
install(DIRECTORY include/ DESTINATION include)
install(TARGETS commlib DESTINATION lib) 
