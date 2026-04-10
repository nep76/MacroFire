/*
	API Hook table
*/

#ifndef HOOKTABLE_H
#define HOOKTABLE_H

#define MF_CTRL_PEEK_BUFFER_POSITIVE    0
#define MF_CTRL_PEEK_BUFFER_NEGATIVE    1
#define MF_CTRL_READ_BUFFER_POSITIVE    2
#define MF_CTRL_READ_BUFFER_NEGATIVE    3
#define MF_CTRL_PEEK_LATCH              4
#define MF_CTRL_READ_LATCH              5
#define MF_VSHCTRL_READ_BUFFER_POSITIVE 6

typedef struct {
	struct{
		void **addr;
		void *entrypoint;
	} restore;
	char *module;
	char *library;
	unsigned int nid;
	void *entrypoint;
} MfHookParams;

MfHookParams hook[] = {
	{ { NULL }, "sceController_Service", "sceCtrl",      0x3A622550, mfCtrlPeekBufferPositive },
	{ { NULL }, "sceController_Service", "sceCtrl",      0xC152080A, mfCtrlPeekBufferNegative },
	{ { NULL }, "sceController_Service", "sceCtrl",      0x1F803938, mfCtrlReadBufferPositive },
	{ { NULL }, "sceController_Service", "sceCtrl",      0x60B81F86, mfCtrlReadBufferNegative },
	{ { NULL }, "sceController_Service", "sceCtrl",      0xB1D0E5CD, mfCtrlPeekLatch },
	{ { NULL }, "sceController_Service", "sceCtrl",      0x0B588501, mfCtrlReadLatch },
	{ { NULL }, "sceVshBridge_Driver",   "sceVshBridge", 0xC6395C03, mfVshCtrlReadBufferPositive },
};

#endif
