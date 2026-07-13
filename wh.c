/*
 *	TODO:
 *	- Get cimgui to display a basic window over the game
 *	- Disable LODs so chams can work properly at a distance
 *	- Modify rendering order so that chams render on top(wallhacks!) ---- (over)done
 *	- Add some kind of aimbot/aim assist(ideally silent aim)
 *	- Test on another PC just in case ---- done, works
 *	- Rainbow chams??
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include "primitives/defs.h"
#include "primitives/hook.h"

// Calculate texture hash
uint32_t calculate_fnv1a(const void* data, size_t size) {
    uint32_t hash = 2166136261u;
    const uint8_t* bytes = (const uint8_t*)data;
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

// Array of known enemy texture hashes
uint32_t enemyTextures[] = {0x7E424D2B, 0x09CB61F7, 0x057F0E72, 0x76BACF3D, 0x430DDAD0, 0x655902E9, 0xA7BE49FF, 0xACDFA40E, 0xD950C8F0, 0xE69ADE10, 0xF02DC1EA, 0xF1419981, 0x01F360A7};

bool checkIfEnemy(const void *pixels, GLsizei width, GLsizei height, GLenum format) {
	int bpp = 0;
	if(format == GL_RGB) bpp = 24;
	else if(format == GL_RGBA) bpp = 32;
	else return false;

	size_t pixelDataSize = (size_t)width * (size_t)height * (size_t)(bpp / 8);
	
	uint32_t textureHash = calculate_fnv1a(pixels, pixelDataSize);

	for(int i = 0;i < sizeof(enemyTextures) / sizeof(enemyTextures[0]);i++) {
		if(enemyTextures[i] == textureHash) {
			clientlogf("[GL HOOK] Enemy is rendered on screen!");
			return true;
		}
	}
	return false;
}	

// Hooked OGL function
void glTexImage2D(GLenum target, int level, int internalformat,
                  GLsizei width, GLsizei height, int border,
                  GLenum format, GLenum type, const void *pixels) {
	
	
	if (target == GL_TEXTURE_2D && level == 0 && pixels) {
		if(checkIfEnemy(pixels, width, height, format)) {
			int bpp = 0;
			if (format == GL_RGB) bpp = 3;
			else if (format == GL_RGBA) bpp = 4;
			else return;

			if (bpp != 0) {
				size_t size = (size_t)width * height * bpp;
				size_t totalPixels = (size_t)width * height;
				
				// Initialize colors
				unsigned char red = 255;
				unsigned char green = 255;
				unsigned char blue = 0;
				unsigned char alpha = 0;

				unsigned char *coloredPixels = malloc(size);
				if(coloredPixels) {
					// Loop through every pixel and set it to the desired color
					for(size_t i = 0;i < totalPixels;i++) {
						size_t offset = i * bpp;
						
						coloredPixels[offset + 0] = red;
						coloredPixels[offset + 1] = green;
						coloredPixels[offset + 2] = blue;

						/* This overflows so 3 bits are enough :3 
						if(bpp = 4) {
							coloredPixels[offset + 3] = alpha;
						}
						*/
					}
				}

				// Forward the call to the real function using the colored texture
				real_glTexImage2D(target, level, internalformat, width, height,
						border, format, type, coloredPixels);

				// Clean up the allocated memory
				free(coloredPixels);
				return;
			}
		}
	}

	if(real_glTexImage2D)
		real_glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

// Hooked SDL function to grab window handle for imgui later
SDL_Window *SDL_CreateWindow(const char* title, int x, int y, int w, int h, Uint32 flags) {
	// Grab real function
	if(!real_SDL_CreateWindow) {
		real_SDL_CreateWindow = dlsym(RTLD_NEXT, "SDL_CreateWindow");
	}
	clientlogf("[SDL Hook] Window title: %s", title);
	if(real_SDL_CreateWindow) {
		windowHandle = real_SDL_CreateWindow(title, x, y, w, h, flags);
		return windowHandle;
	}
}

// Hooked internal funcs
int8_t hooked_isoccluded(float param_1, float param_2, float param_3, float param_4, float param_5) {
	// Make the game think nothing is ever occluded so it always renders models behind walls
	return 0; 
}

void hooked_renderclient(playerent *param_1, char *param_2,char *param_3,int param_4) {
	unhook(renderclient, renderclient_Prologue);
	real_glDepthFunc(GL_ALWAYS);
	
	renderclient(param_1, param_2, param_3, param_4);
	
	hook(renderclient, hooked_renderclient, renderclient_Prologue);
}

__attribute__((constructor))
void initLib() {
	// Initialize everything so that nothing points to NULL
	real_glPolygonMode = dlsym(RTLD_NEXT, "glPolygonMode");
	real_glDepthFunc = dlsym(RTLD_NEXT, "glDepthFunc");
	real_glTexImage2D = dlsym(RTLD_NEXT, "glTexImage2D");
	real_SDL_CreateWindow = dlsym(RTLD_NEXT, "SDL_CreateWindow");

	// Hook internal functions
	hook(renderclient, hooked_renderclient, renderclient_Prologue);
	hook(isoccluded, hooked_isoccluded, isoccluded_Prologue);
}

__attribute__((destructor))
void exitLib() {
	// Unhook everything even though idk if this is ever called
	unhook(isoccluded, isoccluded_Prologue);
	unhook(renderclient, renderclient_Prologue);
}
