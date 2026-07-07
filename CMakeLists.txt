cmake_minimum_required(VERSION 3.20)
project(GitLite VERSION 0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g -pedantic")

set(GLITE_SOURCES
    src/dynarray.c
    src/hashmap.c
    src/blob.c
    src/object_store.c
    src/tree.c
    src/staging_area.c
    src/commit.c
    src/repo.c
    src/queue.c
    src/diff.c
    src/working_dir.c
    src/btree.c
    src/fs_store.c
)

add_executable(glite            src/main.c             ${GLITE_SOURCES})
add_executable(test_commits     src/test_commits.c     ${GLITE_SOURCES})
add_executable(test_branches    src/test_branches.c    ${GLITE_SOURCES})
add_executable(test_diff        src/test_diff.c        ${GLITE_SOURCES})
