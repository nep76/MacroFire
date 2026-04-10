/*
	API Hook table
*/

#ifndef HOOKTABLE_H
#define HOOKTABLE_H

#define MF_CTRL_PEEK_BUFFER_POSITIVE 0
#define MF_CTRL_PEEK_BUFFER_NEGATIVE 1
#define MF_CTRL_READ_BUFFER_POSITIVE 2
#define MF_CTRL_READ_BUFFER_NEGATIVE 3
#define MF_CTRL_PEEK_LATCH           4
#define MF_CTRL_READ_LATCH           5

typedef struct {
	struct {
		void *addr;
		void *entrypoint;
	} exported ;
	char *modname;
	char *library;
	unsigned int nid;
	void *hookfunc;
} MfHookParams;

MfHookParams Hooktable[] = {
	{ { 0, 0 }, "sceController_Service", "sceCtrl", 0x3A622550, mfCtrlPeekBufferPositive },
	{ { 0, 0 }, "sceController_Service", "sceCtrl", 0xC152080A, mfCtrlPeekBufferNegative },
	{ { 0, 0 }, "sceController_Service", "sceCtrl", 0x1F803938, mfCtrlReadBufferPositive },
	{ { 0, 0 }, "sceController_Service", "sceCtrl", 0x60B81F86, mfCtrlReadBufferNegative },
	{ { 0, 0 }, "sceController_Service", "sceCtrl", 0xB1D0E5CD, mfCtrlPeekLatch },
	{ { 0, 0 }, "sceController_Service", "sceCtrl", 0x0B588501, mfCtrlReadLatch }
};

#endif
