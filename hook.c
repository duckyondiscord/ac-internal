#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

// Hardcoded addresses cuz no PIE
#define CLIENTLOGF_ADDR 0x44bf10

// Pointers to the real OGL functions
void (*real_glActiveTextureARB)(unsigned int texture) = NULL;

// Pointer to internal game logging function
typedef void (*clientlogf_t)(const char *format, ...);

// Define internal game logging function
// Hardcoded address cuz no PIE
clientlogf_t clientlogf = (clientlogf_t)CLIENTLOGF_ADDR;

int cnt = 0;

// Hooked OGL function
void hooked_glActiveTextureARB(unsigned int texture) {
	if(cnt < 10) {
		clientlogf("[GL HOOK] glActiveTextureARB hooked!");
		cnt++;
	}
	else if(cnt == 10) {
		clientlogf("[GL HOOK] Stopping logs to not spam console");
		cnt++;
	}

	// Call the real function
	if (real_glActiveTextureARB) {
		real_glActiveTextureARB(texture);
	}
}

void* SDL_GL_GetProcAddress(const char *proc) {
	static void* sdl_handle = NULL;
	void* (*real_SDL_GL_GetProcAddress)(const char*) = NULL;

	sdl_handle = dlopen("libSDL2-2.0.so.0", RTLD_LAZY);

	if(!sdl_handle) {
		dlopen("libSDL2.so", RTLD_LAZY);
	}
	else {
		real_SDL_GL_GetProcAddress = dlsym(sdl_handle, "SDL_GL_GetProcAddress");
	}

	if(!real_SDL_GL_GetProcAddress) {
		printf("CRITICAL: Could not resolve real SDL_GL_GetProcAddress!\n");
		return NULL;
        }	

	if (strcmp(proc, "glActiveTextureARB") == 0) {
        	// Save the actual pointer so the hook can pass execution forward
        	if (!real_glActiveTextureARB) {
			real_glActiveTextureARB = real_SDL_GL_GetProcAddress(proc);
		}
		// Return hooked function addr
		return (void*)&hooked_glActiveTextureARB;
	}

	// For every other OpenGL function, let SDL handle it normally and print the function name
	if (proc) {
        	clientlogf("[SDL HOOK] Game requested: %s", proc);
	}

	return real_SDL_GL_GetProcAddress(proc);
}
