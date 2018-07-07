cmake_minimum_required(VERSION 2.8)

project(vka)

find_package(Vulkan)
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# GLM has some problerms with gcc 7.3 and 8 and requires
# this to be defined.
add_definitions(-DGLM_FORCE_CTOR_INIT)

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


add_custom_target(resources SOURCES ${ResourceFiles})
#add_custom_target(other     SOURCES ${OtherFiles})

INCLUDE_DIRECTORIES( ".")
INCLUDE_DIRECTORIES( "glm")
INCLUDE_DIRECTORIES( ${Vulkan_INCLUDE_DIRS} )

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
                   COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/resources
                   COMMAND ln -s -f ${CMAKE_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources
                   )
