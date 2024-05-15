cmake_minimum_required(VERSION 3.28.1)

project(sfmlopengl)

add_executable(sfmlopengl main.cpp)

target_link_libraries(sfmlopengl sfml-graphics sfml-window sfml-system sfml-audio GLEW GL GLU)

install(TARGETS sfmlopengl RUNTIME DESTINATION bin)
