#include <sys/mman.h>
#include <string.h>
#include "hook.h"



void *getPageAddr(void *addr) {
    return (void *)((uintptr_t)addr & ~(getpagesize() - 1));
}

// Runtime patcher, similar to inline hooking code, avoids the need for a patched game binary
// Also useful for making rust nerds flinch
bool insertRTPatch(void *location, void *patchCode, size_t patchSize) {
	// Make location page writable
	mprotect(getPageAddr(location), getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC);

	// Write patchCode to location
	memcpy(location, patchCode, patchSize);

	// Make location page only executable/readable again
	mprotect(getPageAddr(location), getpagesize(), PROT_READ | PROT_EXEC);

	if (memcmp(location, patchCode, patchSize) == 0) {
		return true;
	}

	return false;
}

// Inline hooking code taken from https://eunomia.dev/blogs/inline-hook/ and modified slightly with the help of gemini to work, licensed under MIT: https://raw.githubusercontent.com/eunomia-bpf/inline-hook-demo/refs/heads/main/LICENSE
static void insertInlineHook(void *origFunc, void *hookFunc) {
	// Write a jump instruction at the start of the original function.
	*((unsigned char *)origFunc + 0) = 0xFF;
	*((unsigned char *)origFunc + 1) = 0x25;
	*((unsigned char *)origFunc + 2) = 0x00;
	*((unsigned char *)origFunc + 3) = 0x00;
	*((unsigned char *)origFunc + 4) = 0x00;
	*((unsigned char *)origFunc + 5) = 0x00;

	// Write the hook function address to the jump target
	*(uint64_t *)((unsigned char *)origFunc + 6) = (uint64_t)hookFunc;
}

void hook(void *origFunc, void *hookFunc, unsigned char origPrologue[SIZE_ORIG_BYTES]) {
	// Store the original bytes of the function.
	memcpy(origPrologue, origFunc, SIZE_ORIG_BYTES);

	// Make the memory page writable.
	mprotect(getPageAddr(origFunc), getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC);

	// As the function name describes, insert the inline hook
	insertInlineHook(origFunc, hookFunc);

	// Make the memory page executable only.
	mprotect(getPageAddr(origFunc), getpagesize(), PROT_READ | PROT_EXEC);
}

void unhook(void *origFunc, unsigned char origPrologue[SIZE_ORIG_BYTES])
{
	// Make the memory page writable.
	mprotect(getPageAddr(origFunc), getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC);

	// Restore the original bytes of the function.
	memcpy(origFunc, origPrologue, SIZE_ORIG_BYTES);

	// Make the memory page executable only.
	mprotect(getPageAddr(origFunc), getpagesize(), PROT_READ | PROT_EXEC);
}
// End of inline hooking code

