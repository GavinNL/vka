cmake_minimum_required(VERSION 2.8)

project(vka)

SET(CMAKE_SKIP_BUILD_RPATH  FALSE)



#FILE(GLOB_RECURSE SourceFiles
#                                "vka/*.h"
#                                "vka/*.hpp"
#                                "examples/*"
#                                "*.inl"
#                                )

FILE(GLOB  TextFiles           "README.md"
                               "TO_DO.md"
                                "Doxygen.cfg"
                                ".gitignore"
                                )
FILE(GLOB_RECURSE ResourceFiles "./resources/*")

INCLUDE_DIRECTORIES(.)

add_definitions(
                 -std=c++17
                -Wall
                #-Werror
                -Wno-error=unused-variable
                -Wno-error=cpp
                -Wno-unused-but-set-variable
                -Wno-error=unused-but-set-variable
                -Wno-error=deprecated-declarations
                -Wno-misleading-indentation
                -Wno-unused-function
                #-Wno-sign-compare
                -Wno-format
                -Wno-unused-variable
                -Wno-narrowing
                -Wno-register
                -Wshadow
                -Werror=return-type
                #-Werror=switch
                )


file(GLOB_RECURSE core_files "src/core/*.cpp")

add_library(vkacore SHARED ${core_files} )

set_target_properties(vkacore PROPERTIES LINKER_LANGUAGE CXX)

add_subdirectory(examples)
add_subdirectory(examples_sdl)


add_custom_command(TARGET vkacore
                   POST_BUILD
                   COMMAND ln -s -f ${CMAKE_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources
                   )
