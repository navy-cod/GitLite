cmake_minimum_required(VERSION 3.20)

project(GitLite VERSION 0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g -pedantic ")

add_executable(glite 
    src/main.c
    src/dynarray.c
    src/hashmap.c
    src/blob.c
    src/object_store.c
)
