cmake_minimum_required(VERSION 3.11)
project(ave3d)


set(PROJECT_DATA_DIR ${PROJECT_SOURCE_DIR}/data)

set(CMAKE_CXX_STANDARD 14)

find_package(PkgConfig REQUIRED)
find_package(OpenGL)

add_subdirectory (deps)
set(GLFW_INCLUDE_DIRS deps/glfw/include/GLFW/)
set(GLFW_LIBRARIES glfw)
set(ASSIMP_INCLUDE_DIRS deps/assimp/include/)
set(ASSIMP_LIBRARIES assimp)

set(srcCommon
    src/common/application.cpp
    src/common/application.hpp
    src/common/image.cpp
    src/common/image.hpp
    src/common/main.cpp
    src/common/mesh.cpp
    src/common/mesh.hpp
    src/common/optimus.cpp
    src/common/renderer.hpp
    src/common/utils.cpp
    src/common/utils.hpp
)

set(srcLibraries
    deps/stb/src/libstb.c
)

set(includePath
    deps/glm/include
    deps/stb/include
)

# OpenGL renderer
if(OpenGL_FOUND)
    set(srcRenderers ${srcRenderers}
        src/opengl.cpp
        src/opengl.hpp
    )
    set(srcLibraries ${srcLibraries}
        deps/glad/src/glad.c
        #deps/glew-1.13.0/src/glew.c
    )
    set(includePath ${includePath}
        deps/glad/include
    )
endif()

add_executable(ave3d ${srcCommon} ${srcLibraries} ${srcRenderers})

#target_compile_features(ave3d PRIVATE cxx_std_14)
target_compile_definitions(ave3d PRIVATE GLFW_INCLUDE_NONE GLM_ENABLE_EXPERIMENTAL ${features})
target_include_directories(ave3d PRIVATE ${includePath} ${GLFW_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS})
target_link_libraries(ave3d ${GLFW_LIBRARIES} ${ASSIMP_LIBRARIES} ${OPENGL_LIBRARIES})

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")  # -fsanitize=address
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")  #-fsanitize=address

set (CMAKE_C_FLAGS_DEBUG "-g -D_DEBUG -fsanitize=address")
set (CMAKE_CXX_FLAGS_DEBUG "-g -D_DEBUG -fsanitize=address")
set (CMAKE_EXE_LINKER_FLAGS_DEBUG "-g -fsanitize=address")

set (CMAKE_C_FLAGS_RELEASE "-O2")
set (CMAKE_CXX_FLAGS_RELEASE "-O2")
set (CMAKE_EXE_LINKER_FLAGS_RELEASE "-O2 -static-libstdc++ -static-libgcc -static")

set (CMAKE_C_FLAGS_RELWITHDEBINFO "-g -O2")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2")
set (CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "-g -O2")

set (CMAKE_C_FLAGS_MINSIZEREL "-Os")
set (CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
set (CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-Os -static-libstdc++ -static-libgcc -static")

install(TARGETS ave3d DESTINATION ${PROJECT_DATA_DIR})
