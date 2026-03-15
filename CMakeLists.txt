cmake_minimum_required(VERSION 3.20)
project(TiledCompressor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find SDL3 first, fall back to SDL2
find_package(SDL3 QUIET CONFIG COMPONENTS SDL3-shared)
if(SDL3_FOUND)
    find_package(SDL3_image QUIET CONFIG)
    if(SDL3_image_FOUND)
        message(STATUS "Found SDL3 and SDL3_image")
        set(SDL_LIBRARY SDL3::SDL3-shared)
        set(SDL_IMAGE_LIBRARY SDL3_image::SDL3_image)
        set(SDL_IMAGE_TARGET SDL3_image::SDL3_image)
        add_compile_definitions(USE_SDL3)
        set(USE_SDL3 TRUE)
    else()
        message(STATUS "SDL3 found but SDL3_image not found, falling back to SDL2")
        set(SDL3_FOUND FALSE)
    endif()
endif()

if(NOT SDL3_FOUND)
    message(STATUS "Using SDL2...")
    find_package(SDL2 REQUIRED CONFIG)
    find_package(SDL2_image REQUIRED CONFIG)
    set(SDL_LIBRARY SDL2::SDL2)
    set(SDL_IMAGE_LIBRARY SDL2_image::SDL2_image)
    set(SDL_IMAGE_TARGET SDL2_image::SDL2_image)
    add_compile_definitions(USE_SDL2)
    set(USE_SDL2 TRUE)
endif()

# Include tmxlite (assuming it's installed or will be added as subdirectory)
# You may need to adjust this based on how tmxlite is installed
find_package(tmxlite QUIET)
if(NOT tmxlite_FOUND)
    # Try to find tmxlite manually (vcpkg doesn't provide CMake config)
    find_path(TMXLITE_INCLUDE_DIR tmxlite/Map.hpp)
    find_library(TMXLITE_LIBRARY NAMES tmxlite tmxlite-d)
    
    if(TMXLITE_INCLUDE_DIR AND TMXLITE_LIBRARY)
        message(STATUS "Found tmxlite manually: ${TMXLITE_LIBRARY}")
        add_library(tmxlite SHARED IMPORTED)
        set_target_properties(tmxlite PROPERTIES
            IMPORTED_LOCATION "${TMXLITE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${TMXLITE_INCLUDE_DIR}"
        )
        # For Windows, we need to set IMPORTED_IMPLIB
        if(WIN32)
            set_target_properties(tmxlite PROPERTIES
                IMPORTED_IMPLIB "${TMXLITE_LIBRARY}"
            )
        endif()
    else()
        # If not found via find_package or manually, use FetchContent
        message(STATUS "tmxlite not found, downloading via FetchContent...")
        include(FetchContent)
        FetchContent_Declare(
            tmxlite
            GIT_REPOSITORY https://github.com/fallahn/tmxlite.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(tmxlite)
    endif()
endif()

# Add stb_image for PNG writing
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Source files
set(SOURCES
    src/main.cpp
    src/MapConverter.cpp
    src/TilesetCompressor.cpp
    src/FileDialog.cpp
)

set(HEADERS
    src/MapConverter.h
    src/TilesetCompressor.h
    src/FileDialog.h
)

add_executable(TiledCompressor ${SOURCES} ${HEADERS})

target_link_libraries(TiledCompressor 
    PRIVATE 
    ${SDL_LIBRARY}
    ${SDL_IMAGE_LIBRARY}
    tmxlite
)

target_include_directories(TiledCompressor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Copy SDL DLL to output directory on Windows
if(WIN32)
    if(USE_SDL3)
        add_custom_command(TARGET TiledCompressor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3-shared>
            $<TARGET_FILE_DIR:TiledCompressor>
        )
        if(TARGET SDL3_image::SDL3_image)
            add_custom_command(TARGET TiledCompressor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:SDL3_image::SDL3_image>
                $<TARGET_FILE_DIR:TiledCompressor>
            )
            # Copy image codec DLLs if using vcpkg
            if(DEFINED VCPKG_TARGET_TRIPLET)
                find_file(LIBPNG_DLL NAMES libpng16.dll PATHS "${CMAKE_PREFIX_PATH}/bin" "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
                find_file(JPEG_DLL NAMES jpeg62.dll PATHS "${CMAKE_PREFIX_PATH}/bin" "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
                if(LIBPNG_DLL)
                    add_custom_command(TARGET TiledCompressor POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${LIBPNG_DLL}"
                        $<TARGET_FILE_DIR:TiledCompressor>
                    )
                endif()
                if(JPEG_DLL)
                    add_custom_command(TARGET TiledCompressor POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${JPEG_DLL}"
                        $<TARGET_FILE_DIR:TiledCompressor>
                    )
                endif()
            endif()
        endif()
    elseif(USE_SDL2)
        add_custom_command(TARGET TiledCompressor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:TiledCompressor>
        )
        if(TARGET SDL2_image::SDL2_image)
            add_custom_command(TARGET TiledCompressor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:SDL2_image::SDL2_image>
                $<TARGET_FILE_DIR:TiledCompressor>
            )
            # Copy image codec DLLs if using vcpkg
            if(DEFINED VCPKG_TARGET_TRIPLET)
                find_file(LIBPNG_DLL NAMES libpng16.dll PATHS "${CMAKE_PREFIX_PATH}/bin" "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
                find_file(JPEG_DLL NAMES jpeg62.dll PATHS "${CMAKE_PREFIX_PATH}/bin" "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
                if(LIBPNG_DLL)
                    add_custom_command(TARGET TiledCompressor POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${LIBPNG_DLL}"
                        $<TARGET_FILE_DIR:TiledCompressor>
                    )
                endif()
                if(JPEG_DLL)
                    add_custom_command(TARGET TiledCompressor POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${JPEG_DLL}"
                        $<TARGET_FILE_DIR:TiledCompressor>
                    )
                endif()
            endif()
        endif()
    endif()
endif()
