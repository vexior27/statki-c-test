cmake_minimum_required(VERSION 3.15)
project(Statki C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Flagi kompilacji
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Katalog z nagłówkami
include_directories(include)

# Pliki źródłowe
set(SOURCES
    src/main.c
    src/game.c
    src/network.c
    src/protocol.c
    src/ai.c
    src/ui.c
)

# Plik wykonywalny
add_executable(statki ${SOURCES})

# Biblioteki sieciowe zależne od systemu operacyjnego
if (WIN32)
    target_link_libraries(statki ws2_32)
endif()
