/* Copyright (c) 2012 Toni Georgiev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "gk.h"
#include "gk_internal.h"
#ifdef WIN32
#define CINTERFACE
#define INITGUID
#include <windows.h>
#include <dinput.h>
#else
#include <X11/Xlib.h>
#endif

gkListenerList *gkMouse = 0;

gkMouseState gkGlobalMouseState;

void gkGetMouseState(gkMouseState* mouseState){
	memcpy(mouseState, &gkGlobalMouseState, sizeof(gkMouseState));
}


void gkSetMousePosition(float x, float y){
#ifdef WIN32
	RECT rect;
	GetWindowRect(gkWindow, &rect);
	SetCursorPos((int)(rect.left + x),(int)(rect.top + y));
#else
    XWarpPointer(display, None, gkWindow, 0,0,0,0, (int)x, (int)y);
#endif
}


gkListenerList *gkKeyboard = 0;

gkKeyboardState gkGlobalKeyboardState;

void gkGetKeyboardState(gkKeyboardState* keyboardState){
	memcpy(keyboardState, & gkGlobalKeyboardState, sizeof(gkKeyboardState));
}

gkJoystick** gkJoysticks;
uint32_t gkJoystickCount;

#if defined(_WIN32)
struct gkJoystickExtStruct{
	wchar_t* name;
	uint8_t flags;
	GUID guid;
	LPDIRECTINPUTDEVICE8 device;
};
typedef struct gkJoystickExtStruct gkJoystickExt;

struct gkJoystickList{
	struct gkJoystickList* next;
	GUID guid;
	wchar_t* name;
};


LPDIRECTINPUT8 gkDI;

void gkInitJoystick(){
	HRESULT hr;
	hr = DirectInput8Create(GetModuleHandle(0), DIRECTINPUT_VERSION, &IID_IDirectInput8, (void**)&gkDI,NULL);
	if(FAILED(hr)){
		printf("Could not create DirectInput8 interface");
		return;
	}
	gkJoysticks = 0;
	gkJoystickCount = 0;
	gkEnumJoysticks();
}

void gkInitDI8Joystick(gkJoystickExt* j, LPDIRECTINPUTDEVICE8 joystick){
	DIPROPRANGE joystickRange;
	DIPROPDWORD deadZone;

	joystick->lpVtbl->SetCooperativeLevel(joystick, gkWindow, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);
	joystick->lpVtbl->SetDataFormat(joystick, &c_dfDIJoystick);

	joystickRange.lMin = -1024;
	joystickRange.lMax = 1024;
	joystickRange.diph.dwSize = sizeof(DIPROPRANGE);
	joystickRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	joystickRange.diph.dwObj = DIJOFS_X;
	joystickRange.diph.dwHow = DIPH_BYOFFSET;
	joystick->lpVtbl->SetProperty(joystick, DIPROP_RANGE, &joystickRange.diph);
	joystickRange.diph.dwObj = DIJOFS_Y;
	joystick->lpVtbl->SetProperty(joystick, DIPROP_RANGE, &joystickRange.diph);
	joystickRange.diph.dwObj = DIJOFS_RX;
	joystick->lpVtbl->SetProperty(joystick, DIPROP_RANGE, &joystickRange.diph);
	joystickRange.diph.dwObj = DIJOFS_RY;
	joystick->lpVtbl->SetProperty(joystick, DIPROP_RANGE, &joystickRange.diph);
	joystickRange.diph.dwObj = DIJOFS_Z;
	joystick->lpVtbl->SetProperty(joystick, DIPROP_RANGE, &joystickRange.diph);
	joystickRange.diph.dwObj = DIJOFS_SLIDER(0);
	joystick->lpVtbl->SetProperty(joystick, DIPROP_RANGE, &joystickRange.diph);
	joystickRange.diph.dwObj = DIJOFS_SLIDER(1);
	joystick->lpVtbl->SetProperty(joystick, DIPROP_RANGE, &joystickRange.diph);

	deadZone.diph.dwSize = sizeof(DIPROPDWORD);
	deadZone.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	deadZone.diph.dwHow = DIPH_BYOFFSET;
	deadZone.diph.dwObj = DIJOFS_X;
	deadZone.dwData = 1000;
	joystick->lpVtbl->SetProperty(joystick, DIPROP_DEADZONE, &deadZone.diph);
	deadZone.diph.dwObj = DIJOFS_Y;
	joystick->lpVtbl->SetProperty(joystick, DIPROP_DEADZONE, &deadZone.diph);
	deadZone.diph.dwObj = DIJOFS_RX;
	joystick->lpVtbl->SetProperty(joystick, DIPROP_DEADZONE, &deadZone.diph);
	deadZone.diph.dwObj = DIJOFS_RY;
	joystick->lpVtbl->SetProperty(joystick, DIPROP_DEADZONE, &deadZone.diph);

	joystick->lpVtbl->Acquire(joystick);
	j->device = joystick;
}

BOOL CALLBACK gkJoystickCallback(LPDIDEVICEINSTANCE device, LPVOID c){
	size_t nameLength = wcslen(device->tszProductName) + 1;
	struct gkJoystickList* lst = c, *n;
	n = (struct gkJoystickList*)malloc(sizeof(struct gkJoystickList));
	n->next = 0;
	n->guid = device->guidInstance;
	n->name = (wchar_t*)malloc(nameLength*sizeof(wchar_t));
	wcscpy(n->name, device->tszProductName);
	lst->next = n;
	return DIENUM_CONTINUE;
}

uint32_t gkEnumJoysticks(){
	uint32_t result = 0, i = 0, u = 0;
	struct gkJoystickList first, *p;
	gkJoystick** tmp;
	first.next = 0;
	gkDI->lpVtbl->EnumDevices(gkDI, DI8DEVCLASS_GAMECTRL, gkJoystickCallback, &first, DIEDFL_ATTACHEDONLY);
	for(p = first.next; p; p = p->next) result++;
	tmp = (gkJoystick**)calloc(result, sizeof(gkJoystick*));
	for(p = first.next; p; p = p->next){
		if(gkJoystickCount){
			for(u = 0; u<gkJoystickCount; u++){
				GUID jguid = ((gkJoystickExt*)gkJoysticks[u])->guid;
				if(IsEqualGUID( &jguid , &p->guid)){
					tmp[i] = gkJoysticks[u];
					gkJoysticks[u] = 0;
					break;
				}
			}
		}else{
			LPDIRECTINPUTDEVICE8 joystick;
			HRESULT hr = gkDI->lpVtbl->CreateDevice(gkDI, &p->guid, &joystick, NULL);
			if(FAILED(hr)){
				printf("Could not create Direct Input device");
			}else{
				gkJoystickExt* j = (gkJoystickExt*)malloc(sizeof(gkJoystickExt));
				j->name = p->name;
				j->flags = 0;
				if(wcscmp(j->name, L"XUSB Gamepad (Controller)") == 0){
					j->flags |= GK_JOYSTICK_XBOX360;
				}
				j->guid = p->guid;
				gkInitDI8Joystick(j, joystick);
				tmp[i] = (gkJoystick*)j;
			}
		}
		i++;
	}
	for(u = 0; u<gkJoystickCount; u++){
		if(gkJoysticks[u]){
			free(gkJoysticks[u]->name);
			free(gkJoysticks[u]);
		}
	}
	free(gkJoysticks);
	gkJoysticks = tmp;
	gkJoystickCount = result;
	return result;
}

void gkGetJoystickState(gkJoystick* joystick, gkJoystickState* state){
	DIJOYSTATE joystate;
	LPDIRECTINPUTDEVICE8 device = ((gkJoystickExt*)joystick)->device;
	int i;

	if(FAILED(device->lpVtbl->Poll(device))){
		device->lpVtbl->Acquire(device);
	}
	if(SUCCEEDED(device->lpVtbl->GetDeviceState(device, sizeof(DIJOYSTATE), &joystate))){
		state->left.x = (float)joystate.lX/1024.0f;
		state->left.y = (float)joystate.lY/1024.0f;
		state->left.z = (float)joystate.lZ/1024.0f;
		state->right.x = (float)joystate.lRx/1024.0f;
		state->right.y = (float)joystate.lRy/1024.0f;
		state->right.z = (float)joystate.lRz/1024.0f;
		for(i = 0; i<32; i++){
			state->buttons[i] = (joystate.rgbButtons[i]&0x80)>0;
		}
		for(i = 0; i<4; i++){
			state->pov[i] = joystate.rgdwPOV[i];
		}
		for(i = 0; i<2; i++){
			state->sliders[i] = (float)joystate.rglSlider[0]/1024.0f;
		}
	}
}

void gkCleanupJoystick(){
	uint32_t i = 0;
	for(i = 0; i<gkJoystickCount; i++){
		gkJoystickExt* j = (gkJoystickExt*)gkJoysticks[i];
		j->device->lpVtbl->Unacquire(j->device);
		j->device->lpVtbl->Release(j->device);
		free(j->name);
		free(j);
	}
	free(gkJoysticks);
	gkJoystickCount = 0;
	if(gkDI){
		gkDI->lpVtbl->Release(gkDI);
	}
}

#else
void gkInitJoystick()
{
}

void gkCleanupJoystick()
{
}
#endif
