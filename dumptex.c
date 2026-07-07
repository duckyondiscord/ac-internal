// Entirely written by Gemini because I cba with image stuff
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>

// OpenGL Type Definitions
typedef unsigned int GLenum;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef unsigned char GLubyte;
#define GL_TEXTURE_2D 0x0DE1
#define GL_RGB 0x1907
#define GL_RGBA 0x1908

#pragma pack(push, 1)
struct TGAHeader {
    uint8_t  idLength;
    uint8_t  colorMapType;
    uint8_t  imageType;
    uint16_t colorMapStart;
    uint16_t colorMapLength;
    uint8_t  colorMapDepth;
    uint16_t xOffset;
    uint16_t yOffset;
    uint16_t width;
    uint16_t height;
    uint8_t  bitsPerPixel;
    uint8_t  descriptor;
};
#pragma pack(pop)

uint32_t calculate_fnv1a(const void* data, size_t size) {
    uint32_t hash = 2166136261u;
    const uint8_t* bytes = (const uint8_t*)data;
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

void dump_texture_to_tga(GLsizei width, GLsizei height, GLenum format, const void* pixels) {
    if (!pixels || width <= 0 || height <= 0) return;

    int bpp = 0;
    if (format == GL_RGB) bpp = 24;
    else if (format == GL_RGBA) bpp = 32;
    else return; 

    size_t pixel_data_size = (size_t)width * (size_t)height * (size_t)(bpp / 8);
    uint32_t texture_hash = calculate_fnv1a(pixels, pixel_data_size);

    char filename[256];
    snprintf(filename, sizeof(filename), "texture_dumps/%dx%d_ID_%X.tga", width, height, texture_hash);

    if (access(filename, F_OK) == 0) return;

    FILE* f = fopen(filename, "wb");
    if (!f) return;

    struct TGAHeader header = {0};
    header.imageType = 2;
    header.width = (uint16_t)width;
    header.height = (uint16_t)height;
    header.bitsPerPixel = (uint8_t)bpp;
    header.descriptor = (format == GL_RGBA) ? 8 : 0;

    fwrite(&header, sizeof(struct TGAHeader), 1, f);
    fwrite(pixels, pixel_data_size, 1, f);
    fclose(f);

    const char *msg = "[DUMP] Texture saved: ";
    write(STDOUT_FILENO, msg, 22);
    write(STDOUT_FILENO, filename, strlen(filename));
    write(STDOUT_FILENO, "\n", 1);
}

// 1. EXPORT THE FUNCTION DIRECTLY
// The dynamic linker will route all game calls here first.
void glTexImage2D(GLenum target, int level, int internalformat, 
                  GLsizei width, GLsizei height, int border, 
                  GLenum format, GLenum type, const void *pixels) {
    
    // Fetch the real OpenGL driver's function pointer using RTLD_NEXT
    static void (*real_glTexImage2D)(GLenum, int, int, GLsizei, GLsizei, int, GLenum, GLenum, const void*) = NULL;
    if (!real_glTexImage2D) {
        real_glTexImage2D = dlsym(RTLD_NEXT, "glTexImage2D");
    }

    // Run the dumper logic
    if (target == GL_TEXTURE_2D && level == 0 && pixels) {
        dump_texture_to_tga(width, height, format, pixels);
    }

    // Pass the call to the real driver
    if (real_glTexImage2D) {
        real_glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    }
}

// Keep your constructor to handle directory creation
__attribute__((constructor))
void init_dumper() {
    mkdir("texture_dumps", 0777);
    write(STDOUT_FILENO, "[INJECTED] Direct texture hook activated.\n", 42);
}
