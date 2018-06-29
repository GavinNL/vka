cmake_minimum_required(VERSION 2.8)

find_package(Vulkan)

#============================================
# For GLFW3 examples
#============================================
find_package(glfw3 3.2 REQUIRED)
#============================================

SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

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

SET(LIBS vkacore glfw vulkan)

file(GLOB files "*.cpp")
foreach(file ${files})
    string(REPLACE "${CMAKE_SOURCE_DIR}/examples/" "" file ${file})
    string(REPLACE ".cpp" "" file ${file})
    #MESSAGE(${file})
    add_executable( ${file} "${file}.cpp")
    target_link_libraries(${file} ${LIBS} )

    set_target_properties( ${file}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
endforeach()
