/*
 *	This is the file that holds every definition, other than custom functions, which
 *	go in specialized headers with accompanying source files.
 */


// OpenGL Type Definitions
typedef unsigned int GLenum;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef unsigned char GLubyte;
typedef bool GLboolean;
#define GL_TEXTURE_2D		0x0DE1
#define GL_RGB			0x1907
#define GL_RGBA			0x1908
#define GL_NEVER		0x0200
#define GL_LESS			0x0201
#define GL_EQUAL		0x0202
#define GL_LEQUAL		0x0203
#define GL_GREATER		0x0204
#define GL_NOTEQUAL		0x0205
#define GL_GEQUAL		0x0206
#define GL_ALWAYS		0x0207
#define GL_FRONT		0x0404
#define GL_BACK			0x0405
#define GL_FRONT_AND_BACK	0x0408
#define GL_POINT		0x1B00
#define GL_LINE			0x1B01
#define GL_FILL			0x1B02

// SDL Type Definitions
typedef uint32_t Uint32;
typedef struct SDL_Window SDL_Window;

// Define window handle that we want to maintain
SDL_Window *windowHandle = NULL;

// Pointers to real OGL functions
static void (*real_glTexImage2D)(GLenum, int, int, GLsizei, GLsizei, int, GLenum, GLenum, const void*) = NULL;
void (*real_glDepthFunc)(GLenum) = NULL;
void (*real_glPolygonMode)(GLenum, GLenum) = NULL;

// Pointers to real SDL functions
SDL_Window * (*real_SDL_CreateWindow)(const char*, int, int, int, int, Uint32) = NULL;

// Hardcoded addresses cuz no PIE
#define CLIENTLOGF_ADDR		0x44bf10
#define ISOCCLUDED_ADDR		0x519b50
#define RENDERCLIENT_ADDR	0x49b8f0

// Internal game structs with unknown fields
typedef struct playerent_t playerent;
typedef struct modelattach_t modelattach;
typedef struct vec_t vec;

// Pointer to internal game functions we want to use/hook
typedef void (*clientlogf_t)(const char *format, ...);
typedef int8_t (*isoccluded_t)(float param_1, float param_2, float param_3, float param_4, float param_5);
typedef void (*renderclient_t)(playerent *param_1, char *param_2, char *param_3, int param_4);

// Define internal game functions
clientlogf_t clientlogf = (clientlogf_t)CLIENTLOGF_ADDR;
isoccluded_t isoccluded = (isoccluded_t)ISOCCLUDED_ADDR;
renderclient_t renderclient = (renderclient_t)RENDERCLIENT_ADDR;

#define SIZE_ORIG_BYTES 14 // Size of a 64-bit long JMP.

// Variables to hold function prologues for inline hooked functions
unsigned char rendermodel_Prologue[SIZE_ORIG_BYTES];
unsigned char renderclient_Prologue[SIZE_ORIG_BYTES];
unsigned char isoccluded_Prologue[SIZE_ORIG_BYTES]; // Never used but I would feel horrible for not including this

