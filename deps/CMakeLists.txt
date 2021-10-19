if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
endif()


add_definitions(
	-D_CRT_SECURE_NO_WARNINGS
)

### GLFW ###

add_subdirectory (glfw)
set(GLFW_INCLUDE_DIRS deps/glfw/include/GLFW/)
set(GLFW_LIBRARIES glfw)

include_directories(
	glfw/include/GLFW/
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(OPENGL_LIBRARY
	${OPENGL_LIBRARY}
	-lGL -lGLU -lXrandr -lXext -lX11 -lrt
	${CMAKE_DL_LIBS}
	${GLFW_LIBRARIES}
)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(OPENGL_LIBRARY
	${OPENGL_LIBRARY}
	${CMAKE_DL_LIBS}
	${GLFW_LIBRARIES}
)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")


if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(
        -D_UNIX
    )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(
        -D_WINDOWS
    )
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

### ASSIMP ###
# AssImp already has a CMakeLists.txt so let's use these

# Compile built-in, modified version of Zlib
include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckFunctionExists)
#add_subdirectory( assimp/contrib/zlib )

set(ZLIB_LIBRARIES zlib)
set(BUILD_STATIC_LIB ON)
set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib)
add_subdirectory( assimp )
