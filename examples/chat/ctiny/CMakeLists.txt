cmake_minimum_required(VERSION 3.10)
project(bakelite_chat_ctiny C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find bakelite code generator
find_program(BAKELITE_EXECUTABLE bakelite REQUIRED)

# Protocol definition
set(PROTO_DEF "${CMAKE_CURRENT_SOURCE_DIR}/../chat.bakelite")

# Generated files
set(GENERATED_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/proto.h")
set(GENERATED_RUNTIME "${CMAKE_CURRENT_SOURCE_DIR}/bakelite.h")

# Generate protocol header from .bakelite file
add_custom_command(
    OUTPUT ${GENERATED_PROTO}
    COMMAND ${BAKELITE_EXECUTABLE} gen -l ctiny -i ${PROTO_DEF} -o ${GENERATED_PROTO}
    DEPENDS ${PROTO_DEF}
    COMMENT "Generating proto.h from chat.bakelite"
)

# Generate runtime header
add_custom_command(
    OUTPUT ${GENERATED_RUNTIME}
    COMMAND ${BAKELITE_EXECUTABLE} runtime -l ctiny -o ${GENERATED_RUNTIME}
    COMMENT "Generating bakelite.h runtime"
)

# Custom target for generated files
add_custom_target(generate_protocol DEPENDS ${GENERATED_PROTO} ${GENERATED_RUNTIME})

add_executable(server server.c)
add_executable(client client.c)

add_dependencies(server generate_protocol)
add_dependencies(client generate_protocol)
