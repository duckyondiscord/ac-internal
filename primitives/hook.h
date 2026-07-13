// Definitions for inline hooking and related stuff.

#define SIZE_ORIG_BYTES 14 // Size of a 64-bit long JMP.

void *getPageAddr(void *addr);
bool insertRTPatch(void *location, void *patchCode, size_t patchSize);
static void insertInlineHook(void *origFunc, void *hookFunc);
void hook(void *origFunc, void *hookFunc, unsigned char origPrologue[SIZE_ORIG_BYTES]);
void unhook(void *origFunc, unsigned char origPrologue[SIZE_ORIG_BYTES]);
