/* Copyright (C) 2015-2017 NY00123
 *
 * This file is part of Reflection Keen.
 *
 * Reflection Keen is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// This header should be used by be_st_sdl* code internally

#ifndef BE_ST_SDL_PRIVATE_H
#define BE_ST_SDL_PRIVATE_H

#include "SDL_version.h" // For SDL_RENDER_DEVICE_RESET hack
#include "SDL_surface.h"

#ifdef REFKEEN_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
#define BEL_ST_SleepMS emscripten_sleep
#else
#define BEL_ST_SleepMS SDL_Delay
#endif

#define BE_ST_MAXJOYSTICKS 8

#define BE_ST_SDL_CONTROLLER_DELAY_BEFORE_DIGIACTION_REPEAT_MS 500
#define BE_ST_SDL_CONTROLLER_DIGIACTION_REPEAT_RATE_MS 40

// Hack for compilation against SDL pre-2.0.4
#if (SDL_PATCHLEVEL < 4)
#define SDL_RENDER_DEVICE_RESET 0x2001
#endif

#define BE_ST_FILL_AUDIO_IN_MAIN_THREAD 1

typedef struct {
	bool isSpecial; // Scancode of 0xE0 sent?
	uint8_t dosScanCode;
} emulatedDOSKeyEvent;

typedef enum {
	BE_ST_MOUSEMODE_ABS_WITH_CURSOR, BE_ST_MOUSEMODE_ABS_WITHOUT_CURSOR, BE_ST_MOUSEMODE_REL
} BESDLMouseModeEnum;

void BEL_ST_ForceHostDisplayUpdate(void);
void BEL_ST_SetGfxOutputRects(bool allowResize);
void BEL_ST_UpdateHostDisplay(void);
void BEL_ST_HandleEmuKeyboardEvent(bool isPressed, bool isRepeated, emulatedDOSKeyEvent keyEvent);
void BEL_ST_ConditionallyShowAltInputPointer(void);
void BEL_ST_SetMouseMode(BESDLMouseModeEnum mode);
// A set of texture management wrappers, used to support recreation of textures on an SDL_RENDER* event.
//
// ***WARNING*** - You MUST pass a pTexture value which is also used
// to manage the texture later, and NOT store a copy of *pTexture in
// another variable.
//
// ***WARNING 2*** - scaleQuality MUST be a C string literal; The string's characters ARE NOT copied!
//
// The thing is, that the SDL_Texture * values may be CHANGED at ANY MOMENT (due to an SDL_RENDER event).
void BEL_ST_SDLCreateTextureWrapper(SDL_Texture **pTexture, Uint32 format, int access, int w, int h, const char *scaleQuality);
void BEL_ST_SDLDestroyTextureWrapper(SDL_Texture **pTexture);
void BEL_ST_RecreateAllTextures(void);

void BE_ST_PrepareForManualAudioCallbackCall(void);

extern SDL_Surface *g_be_sdl_windowIconSurface;
// NOT NECESSARILY AVAILABLE FOR ALL GAMES!
extern bool g_sdlDoAbsMouseMotion;

#endif // BE_ST_SDL_PRIVATE_H
