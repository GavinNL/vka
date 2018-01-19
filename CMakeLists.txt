cmake_minimum_required(VERSION 2.8)

project(vka)

find_package(Vulkan)
find_package(glfw3 3.2 REQUIRED)

FILE(GLOB_RECURSE SourceFiles
                                "vka/*.h"
                                "vka/*.hpp"
                                "examples/*"
                                "*.inl"
                                )

FILE(GLOB  TextFiles           "README.md"
                               "TO_DO.md"
                                "Doxygen.cfg"
                                ".gitignore"
                                )
FILE(GLOB_RECURSE ResourceFiles "./resources/*")

add_custom_target(+1.SOURCE_FILES      SOURCES ${SourceFiles})
add_custom_target(+2.RESOURCES_FILES   SOURCES ${ResourceFiles} )
add_custom_target(+3.TEXT_FILES        SOURCES ${TextFiles} )


set(DROP_DOWN_TEST "VALUE1" CACHE STRING "Solver type VALUE1|VALUE2")
set_property(CACHE DROP_DOWN_TEST PROPERTY STRINGS "VALUE1" "VALUE2")


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

#file(GLOB_RECURSE utils_files "src/utils/*.cpp")
#add_library(utils SHARED ${utils_files} )
#set_target_properties(utils PROPERTIES LINKER_LANGUAGE CXX)
#
#file(GLOB_RECURSE stb_files "src/stb/*.cpp")
#add_library(stb SHARED ${stb_files} )
#set_target_properties(stb PROPERTIES LINKER_LANGUAGE CXX)

#SET(LIBS glfw vulkan core utils stb)
SET(LIBS vkacore glfw vulkan)


#file(GLOB files "*.cpp")
#foreach(file ${files})
#    string(REPLACE "${CMAKE_SOURCE_DIR}/" "" file ${file})
#    string(REPLACE ".cpp" "" file ${file})
#    MESSAGE(${file})
#    add_executable( ${file} "${file}.cpp")
#    target_link_libraries(${file} ${LIBS} )
#
#    set_target_properties( ${file}
#        PROPERTIES
#        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
#    )
#endforeach()

file(GLOB files "examples/*.cpp")
foreach(file ${files})
    string(REPLACE "${CMAKE_SOURCE_DIR}/examples/" "" file ${file})
    string(REPLACE ".cpp" "" file ${file})
    MESSAGE(${file})
    add_executable( ${file} "examples/${file}.cpp")
    target_link_libraries(${file} ${LIBS} )

    set_target_properties( ${file}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
endforeach()


file(GLOB files "tests/*.cpp")
foreach( file ${files} )
    string(REPLACE "${CMAKE_SOURCE_DIR}/tests/" "" file ${file})
    string(REPLACE ".cpp" "" file ${file})
    MESSAGE( ${file} )
    add_executable( ${file} "tests/${file}.cpp" )
    target_link_libraries(${file} ${LIBS} )

    set_target_properties( ${file}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )
endforeach()
