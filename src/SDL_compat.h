// TiledCompressor - Tiled Map Tileset Optimizer
// Copyright (c) 2026 TiledCompressor Contributors
// Licensed under the MIT License - see LICENSE file for details

#pragma once

// SDL Compatibility Layer - supports both SDL2 and SDL3
// NOTE: Only include SDL_main.h in your main.cpp file, not in other source files

#ifdef USE_SDL3
    #include <SDL3/SDL.h>
    // Don't include SDL_main.h here - it should only be in main.cpp
    #include <SDL3_image/SDL_image.h>
    
    // SDL3_image no longer requires initialization
    #define IMG_INIT_PNG 1
    inline int IMG_Init(int flags) { return flags; }  // Always succeed in SDL3
    inline void IMG_Quit() { }  // No-op in SDL3
    inline const char* IMG_GetError() { return SDL_GetError(); }
    
    // SDL3 API changes - SDL_BlitSurface returns bool now
    inline int SDL_BlitSurface_Compat(SDL_Surface* src, const SDL_Rect* srcrect, 
                                       SDL_Surface* dst, SDL_Rect* dstrect) {
        return SDL_BlitSurface(src, srcrect, dst, dstrect) ? 0 : -1;
    }
    
    #define SDL_BlitSurface SDL_BlitSurface_Compat
    
#elif defined(USE_SDL2)
    #include <SDL2/SDL.h>
    // Don't include SDL_main.h here - it should only be in main.cpp
    #include <SDL2/SDL_image.h>
    
    // SDL2 to SDL3 compatibility macros
    #define SDL_CreateSurface SDL_CreateRGBSurfaceWithFormat
    #define SDL_DestroySurface SDL_FreeSurface
    #define SDL_FillSurfaceRect SDL_FillRect
    #define SDL_BlitSurface SDL_BlitSurface
    #define SDL_ConvertSurface SDL_ConvertSurface
    #define SDL_MUSTLOCK SDL_MUSTLOCK
    #define SDL_LockSurface SDL_LockSurface
    #define SDL_UnlockSurface SDL_UnlockSurface
    #define SDL_PIXELFORMAT_RGBA32 SDL_PIXELFORMAT_RGBA32
    
    // Function wrappers for API differences
    inline SDL_Surface* SDL_CreateSurface_Compat(int width, int height, Uint32 format) {
        return SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, format);
    }
    
    inline bool SDL_Init_Compat(Uint32 flags) {
        return SDL_Init(flags) == 0;
    }
    
    inline Uint32 SDL_MapSurfaceRGBA(SDL_Surface* surface, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
        return SDL_MapRGBA(surface->format, r, g, b, a);
    }
    
    // Redefine macros to use compat versions where needed
    #undef SDL_CreateSurface
    #define SDL_CreateSurface SDL_CreateSurface_Compat
    
    // SDL_Init returns 0 on success in SDL2, but bool in SDL3
    #undef SDL_Init
    #define SDL_Init SDL_Init_Compat
    
#else
    #error "Neither USE_SDL2 nor USE_SDL3 is defined. Check CMakeLists.txt"
#endif
