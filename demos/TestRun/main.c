﻿#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if !defined(WIN32) && !defined(_WIN32)
#include <unistd.h>
#endif
#include <gk.h>

#include <GL/gl.h>


#define MY_EVENT 1123
GK_EVENT(MyEvent)
	char msg[100];
GK_EVENT_END()


#define MAX_LINES 100
gkPoint lines[MAX_LINES];

struct drawer{
	GK_BOOL drawing;
};

#define _USE_MATH_DEFINES
#include <math.h>

gkPanel* ppp;
gkImage* img, *img2;
gkFont* font;
float mx, my;

#define STAR_COUNT 2500

typedef struct{
	int id;
	gkPoint stars[STAR_COUNT];
	gkPoint vel[STAR_COUNT];
	float rad[STAR_COUNT];
}PData;

void myDrawFunc(gkPanel* p){
	int i = 1;
	gkMatrix m;
	gkMouseState s;
	gkKeyboardState kb;
	PData* pdata;
	pdata = (PData*)p->data;
	gkGetMouseState(&s);
	gkGetKeyboardState(&kb);
	gkSetLineWidth(s.wheel);
	if(kb.keys[GK_KEY_R]){
		gkSetLineColor(1,0,0,1);
	}else{
		gkSetLineColor(1,1,1,1);
	}
	gkSetLineStipple(1,0xffff);
	gkSetFillColor(0,0,1,1);
	m = gkMatrixCreateIdentity();
	gkDrawPath(lines, MAX_LINES, GK_FALSE);
	gkSetLineWidth(0);
	if(pdata->id == 0){
		gkSetFillColor(1,0,0,0.5);
	}else if(pdata->id == 1){
		gkSetFillColor(0,1,0,0.5);
	}else if(pdata->id == 2){
		gkPoint pp;
		gkSetFillColor(0,0,1,0.5);
		gkSetLineColor(0,0,1,1);
		gkSetLineWidth(2);
		pp = gkTransformPoint(GK_POINT(0,0), &m);
		gkDrawLine(0,0, pp.x, pp.y);
		gkSetLineWidth(0);
	}
	if(p->mouseOver){
		gkSetLineWidth(1);
		gkSetLineColor(1,1,1,1);
		gkSetLineStipple(1,0xf0f0);
	}
	gkDrawRoundRect(0, 0, p->width, p->height,15,15);
	if(p->mouseOver && pdata->id == 0){
		for(i = 0; i<STAR_COUNT; i++){
			float vx = pdata->vel[i].x, vy = pdata->vel[i].y;
			float len = sqrtf(vx*vx + vy*vy);
			gkSetLineWidth(0);
			gkSetFillColor(1,0.8,0,(1.0f - len/2.0f)*0.8f);
			gkDrawPoint(pdata->stars[i].x, pdata->stars[i].y, pdata->rad[i]);
		}
		gkDrawImage(img, p->mouseX + 10.0f,p->mouseY + 10.0f);
		gkDrawImage(img2, mx, my);
	}
	if(pdata->id == 0){
		wchar_t* str = L"Изображение0382\nWA and a new line\nmfor\nfor\nVictory!!\nmore\nand\nmore";
		gkTextFormat tf = gkDefaultTextFormat;
		tf.textColor = GK_COLOR(1,1,1,1);
		tf.strokeColor = GK_COLOR(0,0,0,1.0f);
		tf.strokeSize = 3;
		tf.lineSpacing = -30;
		tf.width = gkGetScreenSize().width;
		tf.height = gkGetScreenSize().height;
		tf.align = GK_TEXT_ALIGN_CENTER;
		tf.valign = GK_TEXT_VALIGN_MIDDLE;
		tf.underline = GK_TRUE;
		gkDrawText(font, str, 0, 0, &tf);
	}
}

#include <GL/gl.h>
float rotateCubeX = 0;
float rotateCubeY = 0;
void drawGL(gkPanel* p){
	float fov = 90.0f, zNear = 1.0f, zFar = 1000.0f, aspect = p->width/p->height;
	float m[16];
	const float h = 1.0f/tan(fov*(M_PI/360.0f));
 float neg_depth = zNear-zFar;
 rotateCubeX+=1.0f;
 rotateCubeY+=1.0f;

m[0] = h / aspect;
 m[1] = 0;
 m[2] = 0;
 m[3] = 0;
m[4] = 0;
 m[5] = h;
 m[6] = 0;
 m[7] = 0;
m[8] = 0;
 m[9] = 0;
 m[10] = (zFar + zNear)/neg_depth;
 m[11] = -1;
m[12] = 0;
 m[13] = 0;
 m[14] = 2.0f*(zNear*zFar)/neg_depth;
 m[15] = 0;

 glEnable(GL_DEPTH_TEST);
 glDepthFunc(GL_LEQUAL);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMultMatrixf(m);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0,0,-5.0f);
	glRotatef(rotateCubeX, 0, 1.0f, 0);
	glRotatef(rotateCubeY, 1.0f, 0, 0);
	glColor4f(0,0,1.0f,1.0f);

	glBegin(GL_QUADS);                  // Start Drawing The Cube
	glColor3f(0.0f,1.0f,0.0f);          // Set The Color To Green
	glVertex3f( 1.0f, 1.0f,-1.0f);          // Top Right Of The Quad (Top)
	glVertex3f(-1.0f, 1.0f,-1.0f);          // Top Left Of The Quad (Top)
	glVertex3f(-1.0f, 1.0f, 1.0f);          // Bottom Left Of The Quad (Top)
	glVertex3f( 1.0f, 1.0f, 1.0f);          // Bottom Right Of The Quad (Top)
	glColor3f(1.0f,0.5f,0.0f);          // Set The Color To Orange
	glVertex3f( 1.0f,-1.0f, 1.0f);          // Top Right Of The Quad (Bottom)
	glVertex3f(-1.0f,-1.0f, 1.0f);          // Top Left Of The Quad (Bottom)
	glVertex3f(-1.0f,-1.0f,-1.0f);          // Bottom Left Of The Quad (Bottom)
	glVertex3f( 1.0f,-1.0f,-1.0f);          // Bottom Right Of The Quad (Bottom)
	glColor3f(1.0f,0.0f,0.0f);          // Set The Color To Red
	glVertex3f( 1.0f, 1.0f, 1.0f);          // Top Right Of The Quad (Front)
	glVertex3f(-1.0f, 1.0f, 1.0f);          // Top Left Of The Quad (Front)
	glVertex3f(-1.0f,-1.0f, 1.0f);          // Bottom Left Of The Quad (Front)
	glVertex3f( 1.0f,-1.0f, 1.0f);          // Bottom Right Of The Quad (Front)
	glColor3f(1.0f,1.0f,0.0f);          // Set The Color To Yellow
	glVertex3f( 1.0f,-1.0f,-1.0f);          // Bottom Left Of The Quad (Back)
	glVertex3f(-1.0f,-1.0f,-1.0f);          // Bottom Right Of The Quad (Back)
	glVertex3f(-1.0f, 1.0f,-1.0f);          // Top Right Of The Quad (Back)
	glVertex3f( 1.0f, 1.0f,-1.0f);          // Top Left Of The Quad (Back)
	glColor3f(0.0f,0.0f,1.0f);          // Set The Color To Blue
	glVertex3f(-1.0f, 1.0f, 1.0f);          // Top Right Of The Quad (Left)
	glVertex3f(-1.0f, 1.0f,-1.0f);          // Top Left Of The Quad (Left)
	glVertex3f(-1.0f,-1.0f,-1.0f);          // Bottom Left Of The Quad (Left)
	glVertex3f(-1.0f,-1.0f, 1.0f);          // Bottom Right Of The Quad (Left)
	glColor3f(1.0f,0.0f,1.0f);          // Set The Color To Violet
	glVertex3f( 1.0f, 1.0f,-1.0f);          // Top Right Of The Quad (Right)
	glVertex3f( 1.0f, 1.0f, 1.0f);          // Top Left Of The Quad (Right)
	glVertex3f( 1.0f,-1.0f, 1.0f);          // Bottom Left Of The Quad (Right)
	glVertex3f( 1.0f,-1.0f,-1.0f);          // Bottom Right Of The Quad (Right)
glEnd();
}

void initPdata(PData* pdata, int index, gkPanel* panel){
	float k = (float)(rand()%500)/100.0f;
	pdata->stars[index] = GK_POINT(panel->mouseX, panel->mouseY);
	pdata->vel[index].x = ((float)(rand()%100 - 50))*k/100.0f;
	pdata->vel[index].y = ((float)(rand()%100 - 50))*k/100.0f;
	pdata->rad[index] = 2;
}

void updatePanel(gkPanel* panel){
	int i = 0;
	PData* pdata = (PData*)panel->data;

	for(i = 0; i<STAR_COUNT; i++){
		float vx = pdata->vel[i].x, vy = pdata->vel[i].y;
		float len = sqrtf(vx*vx + vy*vy);
		float t = (float)100.0f/(float)gkGetFps();
		if(len<2 && len > 0){
			pdata->stars[i].x += vx*t;
			pdata->stars[i].y += vy*t;
			pdata->vel[i].y += 0.01f*t;
			pdata->rad[i] += 0.1f*t;
		}else{
			initPdata(pdata, i, panel);
		}
	}
	if(pdata->id == 0){
		static float rd = 0;
		if(gkBeginDrawToImage(img, GK_TRUE)){
			gkMatrix mat = gkMatrixCreateTranslation(-100,-100);
			gkMatrixMult(&mat, gkMatrixCreateRotation(rd));
			gkMatrixMult(&mat, gkMatrixCreateTranslation(100,100));
			rd += 0.1f;
			gkSetLineColor(1,1,1,1);
			gkSetLineWidth(2);
			gkSetLineStipple(1, 0xf0f0);
			gkSetFillColor(0,0,0,0.5f);
			gkPushTransform(&mat);
			gkDrawCircle(100,100,50);
			gkPopTransform();
			gkEndDrawToImage();
		}
	}
}

gkPanel*vp;
GK_BOOL onMouseDown(gkEvent* evtPtr, void* p){
	gkPanel* c;
	gkMouseState ms;
	((struct drawer*)p)->drawing = GK_TRUE;
	if(evtPtr->target == gkKeyboard){
		gkKeyboardEvent* evt = (gkKeyboardEvent*)evtPtr;
		if(evt->key.code == GK_KEY_ESCAPE) gkExit();
		printf("%d : %d\n", evt->key.code, evt->key.modifiers);
	}
	gkGetMouseState(&ms);
	gkAddTween(&mx, GK_TWEEN_EASE_OUT_ELASTIC, 1000, GK_FLOAT, mx, ms.position.x);
	gkAddTween(&my, GK_TWEEN_EASE_OUT_ELASTIC, 1000, GK_FLOAT, my, ms.position.y);
	return GK_TRUE;
}
GK_BOOL onMouseUp(gkEvent* evtPtr, void* p){
	((struct drawer*)p)->drawing = GK_FALSE;
	return GK_TRUE;
}

GK_BOOL onMouseMove(gkEvent* evtPtr, void* p){
	gkPanel* panel;
	gkMouseEvent* mevt = (gkMouseEvent*)evtPtr;
	if(p && ((struct drawer*)p)->drawing){
		gkMouseEvent* evt = (gkMouseEvent*)evtPtr;
		MyEvent e;
		gkMouseState s;
		int i = MAX_LINES;
		while(--i>0){
			lines[i] = lines[i-1];
		}
		gkGetMouseState(&s);
		lines[0] = s.position;
//		mresize(ppp, evt->position.x - ppp->x, evt->position.y - ppp->y);
		e.type = MY_EVENT;
		sprintf(e.msg, "myevent: %d", gkGetFps());
		gkDispatch(evtPtr->target, &e);
	}
	panel = (gkPanel*)evtPtr->currentTarget;
	initPdata((PData*)panel->data, 0, panel);
	gkAddTween(&mx, GK_TWEEN_EASE_OUT_SINE, 5000, GK_FLOAT, mx, mevt->position.x);
	gkAddTween(&my, GK_TWEEN_EASE_OUT_SINE, 5000, GK_FLOAT, my, mevt->position.y);
	return GK_TRUE;
}

GK_BOOL onMyEvent(gkEvent* evt, void* p){
	MyEvent* e = (MyEvent*)evt;
	printf("%s\n", e->msg);
	return GK_TRUE;
}

GK_BOOL onPanelResize(gkEvent* evt, void *p){
	gkPanel* panel = (gkPanel*)evt->target;
	printf("panel resized: %d x %d\n", (int)panel->width, (int)panel->height);
	return GK_TRUE;
}

#define _USE_MATH_DEFINES
#include <math.h>

void mresize1(gkPanel* p, float w, float h){
	if(p->numChildren>0){
		gkResizePanel(p, w,h);
		p = gkGetChildAt(p, 1);
		p->x = 10;
		p->y = h - 210;
		gkResizePanel(p, w - 20, 200);
	}
}

GK_BOOL onMouseEnter(gkMouseEvent* evt, void* p){
//	((gkPanel*)evt->currentTarget)->data = 1;
	return GK_TRUE;
}
GK_BOOL onMouseLeave(gkMouseEvent* evt, void* p){
//	((gkPanel*)evt->currentTarget)->data = 2;
	return GK_TRUE;
}

void onKUp(gkKeyboardEvent* evt, void* p){
	wchar_t msgBuff[2550], c[2];
	c[1] =0;
	wcscpy(msgBuff, gkGetWindowTitle());
	c[0] = evt->character;
	wcscat(msgBuff, c);
	gkSetWindowTitle(msgBuff);
}

GK_BOOL onFocusIn(gkEvent* evt, void* p){
	printf("on focus in\n");
	return GK_TRUE;
}
GK_BOOL onFocusOut(gkEvent* evt, void* p){
	printf("on focus out\n");
	return GK_TRUE;
}

void onTimer1(gkEvent* evt, void* p){
//	printf("onTimer1 @ %d\n", (int)gkMilliseconds());
}
void onTimer2(gkEvent* evt, void* p){
//	printf("  onTimer2 @ %d\n", (int)gkMilliseconds());
	if(gkMilliseconds()>6000){
		gkStopTimer((gkTimer*)evt->target);
	}
}

char* styleName(uint8_t style){
	switch(style){
		case GK_FONT_NORMAL: return "Regular";
		case GK_FONT_ITALIC: return "Italic";
		case GK_FONT_BOLD: return "Bold";
		case GK_FONT_BOLD_ITALIC: return "Bold Italic";
	}
	return "unknown";
}
void printResource(gkFontResource* rc){
	int i;
	for(i = 0; i<rc->numFaces; i++){
		printf("Loaded face %s with style %s\n", rc->faces[i]->fontFamily, styleName(rc->faces[i]->style));
	}
}

GK_BOOL onSndStopped(gkEvent* e)
{
    gkSoundEvent* evt = (gkSoundEvent*)e;
    printf("Sound stopped %p %p Restarting\n", e->target, evt->sound);
    gkAddListener( gkPlaySound(evt->sound), GK_ON_SOUND_STOPPED, 0, onSndStopped, 0);
    return GK_TRUE;
}

int main(){
	if(gkInit()){
		gkTimer* timer1, *timer2;
		gkPanel* panel = gkCreatePanel();
		int i;
		struct drawer d;
		PData d1, d2, d3;
		gkPanel* p1 = gkCreatePanel(), *p2 = gkCreatePanel();
		gkFontResource* rc;
		gkSound* snd;

		vp = gkCreateViewportPanel();

		timer1 = gkCreateTimer();
		timer1->delay = 100;
		timer1->repeats = 3;
		timer1->interval = 1000;
		gkAddListener(timer1, GK_ON_TIMER, 0, onTimer1, 0);
		gkStartTimer(timer1, GK_TRUE);

		timer2 = gkCreateTimer();
		timer2->delay = 500;
		timer2->interval = 1000;
		gkAddListener(timer2, GK_ON_TIMER, 0, onTimer2, 0);
		gkStartTimer(timer2, GK_FALSE);

		d1.id = 0; d2.id = 1; d3.id = 2;
		gkResizePanel(panel, 400,400);
		gkAddChild(panel, vp);
		gkAddChild(panel, p1);
		gkAddChild(p1, p2);
		d.drawing = GK_FALSE;
		for(i = 0; i<MAX_LINES; i++){
			lines[i] = GK_POINT(0,0);
		}
		vp->width = 400;
		vp->height = 300;
		vp->x = 0;
		vp->y = 0;
		vp->drawFunc = drawGL;
		ppp = p1;
		p1->width = 200;
		p1->height = 200;
		p1->x = 100;
		p1->y = 10;
		p1->transform = gkMatrixCreateRotation(0.5);
		p2->width = 100;
		p2->height = 100;
		p2->x = 50;
		p2->y = 50;
		p2->transform = gkMatrixCreateRotation(1.5);
		panel->data = &d1;
		vp->autosizeMask = GK_END_LEFT|GK_END_RIGHT|GK_END_TOP|GK_END_BOTTOM;
		p2->autosizeMask = GK_END_LEFT|GK_END_RIGHT|GK_END_TOP|GK_END_BOTTOM;
		p1->autosizeMask = GK_START_LEFT|GK_END_RIGHT|GK_START_TOP|GK_START_BOTTOM;
		panel->resizeFunc = mresize1;
		p1->data = &d2;
		p2->data = &d3;
		p1->drawFunc = myDrawFunc;
		p2->drawFunc = myDrawFunc;
		panel->drawFunc = myDrawFunc;
		p1->updateFunc = updatePanel;
		p2->updateFunc = updatePanel;
		panel->updateFunc = updatePanel;
		gkAddListener(p1, GK_ON_MOUSE_ENTER, 0, onMouseEnter, 0);
		gkAddListener(p1, GK_ON_MOUSE_LEAVE, 0, onMouseLeave, 0);
		gkAddListener(p2, GK_ON_MOUSE_ENTER, 0, onMouseEnter, 0);
		gkAddListener(p2, GK_ON_MOUSE_LEAVE, 0, onMouseLeave, 0);
		gkAddListener(p2, GK_ON_PANEL_FOCUS_OUT, 0, onFocusOut, 0);
		gkAddListener(p2, GK_ON_PANEL_FOCUS_IN, 0, onFocusIn, 0);
//		gkAddListener(p1, GK_ON_KEY_DOWN, 0, onMouseDown, &d);
		gkAddListener(panel, GK_ON_CHARACTER, 0, onKUp, 0);
		gkAddListener(p1, GK_ON_CHARACTER, 0, onKUp, 0);
		gkAddListener(p2, GK_ON_CHARACTER, 0, onKUp, 0);
//		gkAddListener(gkKeyboard, GK_ON_KEY_REPEAT, 0, onMouseDown, &d);
//		gkAddListener(gkKeyboard, GK_ON_KEY_UP, 0, onMouseUp, &d);
		gkAddListener(panel, GK_ON_MOUSE_DOWN, 0, onMouseDown, &d);
		gkAddListener(panel, GK_ON_MOUSE_UP, 0, onMouseUp, &d);
		gkAddListener(panel, GK_ON_MOUSE_MOVE, 0, onMouseMove, &d);
		gkAddListener(p1, GK_ON_MOUSE_MOVE, 0, onMouseMove, 0);
		gkAddListener(p2, GK_ON_MOUSE_MOVE, 0, onMouseMove, 0);
		gkAddListener(p1, MY_EVENT, 0, onMyEvent, 0);
		gkAddListener(panel, GK_ON_PANEL_RESIZED, 0, onPanelResize, 0);
		gkSetMainPanel(panel);
		gkSetWindowTitle(L"Simple panel");
//		gkSetWindowResizable(GK_FALSE);
		gkSetScreenSize(GK_SIZE(1280,720));
		gkSetTargetFps(GK_VSYNC);
//		gkSetFullscreen(GK_TRUE);
        if(gkEnumJoysticks()){
            if((gkJoysticks[0]->flags & GK_JOYSTICK_XBOX360) > 0)
            {
                printf("\tXBox360 controlled found\n");
            }
        }
		gkSetMousePosition(1280/2, 720/2);
		{
			img = gkCreateImage(200,200);
			if(gkBeginDrawToImage(img, GK_TRUE)){
				gkSetLineColor(1,1,1,1);
				gkSetLineWidth(2);
				gkSetLineStipple(2, 0xff00);
				gkSetFillColor(0,0,0,0.5f);
				gkDrawCircle(100,100,50);
				gkEndDrawToImage();
			}
			img2 = gkLoadImage(L"../demos/TestRun/ball.png");
			mx = 0;
			my = 0;
		}
		rc = gkAddFontResource("../demos/TestRun/meiryo.ttc");
		printResource(rc);
		font = gkCreateFont("meiryo", 50, GK_FONT_NORMAL);
//		printResource(gkAddFontResource("../fonts/arial.ttf"));
//		printResource(gkAddFontResource("../fonts/arialbd.ttf"));
//		printResource(rc);
//		printResource(gkAddFontResource("../fonts/ariali.ttf"));
//		gkRemoveFontResource(rc);
//		rc = gkAddFontResource("../fonts/meiryo.ttc");
//		if(font = gkCreateFont("ArIaL", 10, GK_FONT_BOLD)){
//			printf("Font created\n");
//			gkDestroyFont(font);
//		}
//		printResource(rc);
//		gkRemoveFontResource(rc);
        snd = gkLoadSound("../demos/TestRun/cat.wav", GK_SOUND_STATIC);
        gkAddListener( gkPlaySound(snd), GK_ON_SOUND_STOPPED, 0, onSndStopped, 0);

		gkRun();
		gkDestroySound(snd);
		gkRemoveFontResource(rc);
		gkDestroyImage(img2);
		gkDestroyImage(img);
		gkDestroyPanel(panel);
		gkDestroyPanel(p1);
		gkDestroyPanel(p2);
		gkDestroyPanel(vp);
	}
	return 0;
}

/*

gkImage* img;

GK_BOOL toggleFullscreen(gkEvent* evt, void* param)
{
    gkSetFullscreen(!gkIsFullscreen());
    return GK_TRUE;
}

GK_BOOL mouseMove(gkEvent* evt, void* param)
{
    gkMouseEvent* mevt = (gkMouseEvent*)evt;
    printf("fps: %d\n", gkGetFps());
    return GK_TRUE;
}

GK_BOOL drawMe(gkPanel* panel)
{
    gkMatrix scale = gkMatrixCreateScale(0.1, 0.1);
    gkSetFillColor(1,0,0,1);
    gkDrawCircle(100,100, 100);

    gkPushTransform(&scale);
    gkDrawImage(img, 300, 0);
    gkPopTransform();

    return GK_TRUE;
}

int main()
{
    if(gkInit(GK_AUTO))
    {
        int modes, i;
        gkSize* sizes;
        gkPanel* panel;

        panel = gkCreatePanel();
        panel->drawFunc = drawMe;
        gkSetMainPanel(panel);

        img = gkLoadImage(L"/home/khaos/Downloads/Изображение0382.jpg");

        gkSetTargetFps(GK_VSYNC);
        gkSetScreenSize(GK_SIZE(800,600));
//        gkSetScreenSize(GK_SIZE(1920, 1080));
//        gkSetFullscreen(GK_TRUE);
        gkSetWindowResizable(GK_FALSE);
        gkSetWindowTitle(L"My Window");
        modes = gkGetSupportedScreenSizes(0);
        sizes = (gkSize*)calloc(modes, sizeof(gkSize));
        gkGetSupportedScreenSizes(sizes);
        for(i = 0; i<modes; i++)
        {
            printf("size: %f x %f\n", sizes[i].width, sizes[i].height);
        }
        gkAddListener(gkMouse, GK_ON_MOUSE_UP, 0, toggleFullscreen, 0);
        gkAddListener(gkMouse, GK_ON_MOUSE_MOVE, 0, mouseMove, 0);
        free(sizes);
        gkRun();
        gkDestroyImage(img);
        gkDestroyPanel(panel);
    }
    return 0;
}
*/
