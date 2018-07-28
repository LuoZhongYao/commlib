if("${CMAKE_SYSTEM_NAME}" MATCHES "QNX")
    set(os_libs socket)
elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Android")
    set(os_libs log)
    link_directories(${GDK_SOURCE_DIR}/OS/Android/android-${CMAKE_SYSTEM_VERSION})
elseif(UNIX)
    set(os_libs pthread dl)
endif()

add_library(u urwlib.c umod.c getpname.c kfifo.c btsnoop.c)
target_link_libraries(u ${os_libs})

install(TARGETS u DESTINATION lib) 
