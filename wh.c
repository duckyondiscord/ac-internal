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

// OpenGL Type Definitions
typedef unsigned int GLenum;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef unsigned char GLubyte;
#define GL_TEXTURE_2D 0x0DE1
#define GL_RGB 0x1907
#define GL_RGBA 0x1908

// SDL Type Definitions
typedef uint32_t Uint32;
typedef struct SDL_Window SDL_Window;

// Define window handle that we want to maintain
SDL_Window *windowHandle = NULL;

// Pointers to real SDL functions
SDL_Window * (*real_SDL_CreateWindow)(const char*, int, int, int, int, Uint32) = NULL;

// Hardcoded addresses cuz no PIE
#define CLIENTLOGF_ADDR 0x44bf10
#define RENDERENTITIES_ADDR 0x45f870

// Pointer to internal game functions we want to use/hook
typedef void (*clientlogf_t)(const char *format, ...);
typedef void (*renderentities_t)(void);

// Define internal game logging function
// Hardcoded address cuz no PIE
clientlogf_t clientlogf = (clientlogf_t)CLIENTLOGF_ADDR;
renderentities_t renderentities = (renderentities_t)RENDERENTITIES_ADDR; 

// Inline hooking code taken from https://eunomia.dev/blogs/inline-hook/ and modified slightly with the help of gemini to work
#define SIZE_ORIG_BYTES 17 
static void insertInlineHook(void *origFunc, void *hookFunc) {
	// unsigned char *bytes = (unsigned char *)origFunc;
	// printf("First bytes: %02x %02x %02x %02x %02x\n", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4]);
	
	// Write a jump instruction at the start of the original function.
	*((unsigned char *)origFunc + 0) = 0xFF;
	*((unsigned char *)origFunc + 1) = 0x25;
	*((unsigned char *)origFunc + 2) = 0x00;
	*((unsigned char *)origFunc + 3) = 0x00;
	*((unsigned char *)origFunc + 4) = 0x00;
	*((unsigned char *)origFunc + 5) = 0x00;

	// Write the hook function address to the jump target
	*(uint64_t *)((unsigned char *)origFunc + 6) = (uint64_t)hookFunc;
	
	// printf("First bytes after hook: %02x %02x %02x %02x %02x\n", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4]);
	
	//int64_t relativeOffset = (int64_t)((unsigned char *)hookFunc - (unsigned char *)origFunc - 5);
	//*(int64_t *)((unsigned char *)origFunc + 6) = relativeOffset;
	// treat this address as a place where a 64-bit integer belongs, then write the integer to it
}

void *getPageAddr(void *addr) {
    return (void *)((uintptr_t)addr & ~(getpagesize() - 1));
}

unsigned char origBytes[SIZE_ORIG_BYTES];

void hook(void *origFunc, void *hookFunc) {
    // Store the original bytes of the function.
    memcpy(origBytes, origFunc, SIZE_ORIG_BYTES);

    // Make the memory page writable.
    mprotect(getPageAddr(origFunc), getpagesize(),
         PROT_READ | PROT_WRITE | PROT_EXEC);

    insertInlineHook(origFunc, hookFunc);

    // Make the memory page executable only.
    mprotect(getPageAddr(origFunc), getpagesize(),
         PROT_READ | PROT_EXEC);
}

void unhook(void *origFunc)
{
    // Make the memory page writable.
    mprotect(getPageAddr(origFunc), getpagesize(),
         PROT_READ | PROT_WRITE | PROT_EXEC);

    // Restore the original bytes of the function.
    memcpy(origFunc, origBytes, SIZE_ORIG_BYTES);

    // Make the memory page executable only.
    mprotect(getPageAddr(origFunc), getpagesize(),
         PROT_READ | PROT_EXEC);
}
// End of inline hooking code

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

	for(int i = 0;i < sizeof(enemyTextures) / sizeof(enemyTextures[1]);i++) {
		if(enemyTextures[i] == textureHash) {
			clientlogf("[GL HOOK] Enemy is rendered on screen!");
			return true;
		}
	}
	return false;
}	

void hooked_renderentities(void) {
	// We do NOT have to call the real renderentities() because unlike LD_PRELOAD hooking, with inline hooking, our code returns to the original function once done.
	clientlogf("67");
}

// Hooked OGL function
void glTexImage2D(GLenum target, int level, int internalformat,
                  GLsizei width, GLsizei height, int border,
                  GLenum format, GLenum type, const void *pixels) {

	// Get real function pointer
	static void (*real_glTexImage2D)(GLenum, int, int, GLsizei, GLsizei, int, GLenum, GLenum, const void*) = NULL;
	if(!real_glTexImage2D) {
		real_glTexImage2D = dlsym(RTLD_NEXT, "glTexImage2D");
	}
	
	if (target == GL_TEXTURE_2D && level == 0 && pixels) {
		if(checkIfEnemy(pixels, width, height, format)) {
			int bpp = 0;
			if (format == GL_RGB) bpp = 3;
			else if (format == GL_RGBA) bpp = 4;
			else return;
			
			if (bpp != 0) {
				size_t size = (size_t)width * height * bpp;
				size_t totalPixels = (size_t)width * height;
				
				unsigned char red = 255;
				unsigned char green = 255;
				unsigned char blue = 0;
				unsigned char alpha = 255;

				unsigned char *coloredPixels = malloc(size);
				if (coloredPixels) {
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

__attribute__((constructor))
void initLib() {
	hook(renderentities, hooked_renderentities);
}

__attribute__((destructor))
void exitLib() {
	unhook(renderentities);
}
