cmake_minimum_required (VERSION 2.8)
project (multitun)
set(CMAKE_VERBOSE_MAKEFILE ON)
# Versioning
set (SIMPLETUN_VERSION_MAJOR 0)
set (SIMPLETUN_VERSION_MINOR 1)
set (SIMPLETUN_VERSION_PATCH 0)

# Compiler
set (CMAKE_cxx_COMPILER "g++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++14 -Wall -Wextra")

# Define input directories
set (HEADER_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set (SOURCE_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set (LIB_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Define output directories
set (HEADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/include/)

# Includes for compiling (the output of `configure_file`, see below)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)


# Define header files.
set (HEADER_FILES 
	idemux.h
	imux.h
    options.h
	queue.h
	roles.h
    util.h
	tun.h
	socket.h
)

# Configure header files. Read from the input dir, output processed header files in the binary dir.
foreach (file ${HEADER_FILES})
    configure_file (
        "${HEADER_INPUT_DIR}/${file}"
        "${HEADER_OUTPUT_DIR}/${file}"
    )
endforeach (file)


# Include the library files
set (LIB_FILES 
	${LIB_INPUT_DIR}/idemux
	${LIB_INPUT_DIR}/imux
    ${LIB_INPUT_DIR}/options
    ${LIB_INPUT_DIR}/queue
	${LIB_INPUT_DIR}/roles
	${LIB_INPUT_DIR}/socket
	${LIB_INPUT_DIR}/tun
)
set (OTHER_LIBS pthread)

# Define the build targets
add_executable (multitun ${SOURCE_INPUT_DIR}/multitun.cpp ${LIB_FILES})
target_link_libraries(multitun ${OTHER_LIBS})
