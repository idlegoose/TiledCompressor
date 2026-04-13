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
    if(WIN32)
        find_file(TMXLITE_DLL NAMES tmxlite.dll
            PATHS
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin"
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin"
            "${CMAKE_PREFIX_PATH}/bin"
            "${CMAKE_PREFIX_PATH}/debug/bin")
    endif()
    
    if(TMXLITE_INCLUDE_DIR AND TMXLITE_LIBRARY)
        message(STATUS "Found tmxlite manually: ${TMXLITE_LIBRARY}")
        add_library(tmxlite SHARED IMPORTED)
        if(WIN32 AND TMXLITE_DLL)
            set_target_properties(tmxlite PROPERTIES
                IMPORTED_LOCATION "${TMXLITE_DLL}"
                INTERFACE_INCLUDE_DIRECTORIES "${TMXLITE_INCLUDE_DIR}"
            )
        else()
            set_target_properties(tmxlite PROPERTIES
                IMPORTED_LOCATION "${TMXLITE_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${TMXLITE_INCLUDE_DIR}"
            )
        endif()
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

# vcpkg x64-windows may provide release-only tmxlite binaries. On MSVC this can
# break Debug due to STL ABI mismatch across DLL boundaries (tmxlite API uses
# std::string). If no debug tmxlite import lib exists, align Debug ABI to release.
if(MSVC AND DEFINED VCPKG_TARGET_TRIPLET)
    set(TMXLITE_DEBUG_IMPLIB "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib/tmxlite.lib")
    if(NOT EXISTS "${TMXLITE_DEBUG_IMPLIB}")
        message(WARNING "tmxlite debug library not found; aligning Debug runtime ABI to release for compatibility.")
        set_property(TARGET TiledCompressor PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
        target_compile_definitions(TiledCompressor PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)
    endif()
endif()

target_include_directories(TiledCompressor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Copy SDL DLL to output directory on Windows
if(WIN32)
    # Always try to copy tmxlite runtime when available as a target.
    if(TARGET tmxlite)
        add_custom_command(TARGET TiledCompressor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:tmxlite>
            $<TARGET_FILE_DIR:TiledCompressor>
        )
    endif()

    # If using vcpkg, also copy common tmxlite dependencies from bin folders.
    if(DEFINED VCPKG_TARGET_TRIPLET)
        find_file(TMXLITE_DLL_RELEASE NAMES tmxlite.dll
            PATHS
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin"
            "${CMAKE_PREFIX_PATH}/bin")
        find_file(TMXLITE_DLL_DEBUG NAMES tmxlite.dll
            PATHS
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin"
            "${CMAKE_PREFIX_PATH}/debug/bin")
        find_file(PUGIXML_DLL_RELEASE NAMES pugixml.dll
            PATHS
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin"
            "${CMAKE_PREFIX_PATH}/bin")
        find_file(PUGIXML_DLL_DEBUG NAMES pugixml.dll
            PATHS
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin"
            "${CMAKE_PREFIX_PATH}/debug/bin")

        if(TMXLITE_DLL_RELEASE)
            add_custom_command(TARGET TiledCompressor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${TMXLITE_DLL_RELEASE}"
                $<TARGET_FILE_DIR:TiledCompressor>
            )
        endif()
        if(TMXLITE_DLL_DEBUG)
            add_custom_command(TARGET TiledCompressor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${TMXLITE_DLL_DEBUG}"
                $<TARGET_FILE_DIR:TiledCompressor>
            )
        endif()
        if(PUGIXML_DLL_RELEASE)
            add_custom_command(TARGET TiledCompressor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${PUGIXML_DLL_RELEASE}"
                $<TARGET_FILE_DIR:TiledCompressor>
            )
        endif()
        if(PUGIXML_DLL_DEBUG)
            add_custom_command(TARGET TiledCompressor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${PUGIXML_DLL_DEBUG}"
                $<TARGET_FILE_DIR:TiledCompressor>
            )
        endif()
    endif()

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
