cmake_minimum_required(VERSION 3.8)

project (lime)

#find_package (PkgConfig REQUIRED)
#pkg_check_modules (GTK3 REQUIRED gtk+-3.0)
#set(CMAKE_C_STANDARD 11)
#include_directories (${GTK3_INCLUDE_DIRS})
#link_directories (${GTK3_LIBRARY_DIRS})




add_executable(
    lime
    "src/log.c"
    "src/mem.c"
    "src/list.c"
    "src/manager.c"
    "src/main.c"
)
#add_definitions (${GTK3_CFLAGS_OTHER})
target_link_libraries (
    lime 
    #    ${GTK3_LIBRARIES}
    X11
    )
