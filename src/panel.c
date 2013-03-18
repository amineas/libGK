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

#define GK_INTERNAL

#include "gk.h"

#ifdef GK_WIN
#include <windows.h>
#endif

#include <stdlib.h>
#include <GL/gl.h>

static void gkUpdateClientArea(gkClientArea* area, gkClientArea* oldArea, float x, float y, float width, float height)
{
    area->deltaX = (x - oldArea->x);
    area->deltaY = (y - oldArea->y);
    area->deltaWidth = (width - oldArea->width);
    area->deltaHeight = (height - oldArea->height);
    area->x = x;
    area->y = y;
    area->width = width;
    area->height = height;
}

void gkProcessLayoutPanel(gkPanel* panel, gkClientArea* clientArea);
void gkUpdateLayout(gkPanel* panel);

#define FABS(x) x<0?-x:x

/* AutosizeMask layout method */

float resizeEdge(float edge, uint8_t mask, float oldStart, float oldEnd, float start, float end){
	if(mask == 1){
		return edge;
	}else if(mask == 2){
		float offset = (oldEnd - oldStart) - edge;
		return (end - start) - offset;
	}else if(mask == 3){
		float k = edge/(oldEnd - oldStart);
		return (end - start)*k;
	}
	return edge;
}

void gkLayoutFuncAutosize(gkPanel* p, gkClientArea* area)
{
	float left, right, top, bottom, nleft, nright, ntop, nbottom;
	uint16_t autosizeMask = (p->layoutMethod.params[0]);
	left = 0; right = area->width;	top = 0; 	bottom = area->height;

	nleft = resizeEdge(p->x,				 autosizeMask&3,		left, area->width - area->deltaWidth ,left, right);
	nright = resizeEdge(p->x + p->width,	(autosizeMask>>2)&3,	left, area->width - area->deltaWidth, left, right);
	ntop = resizeEdge(p->y,					(autosizeMask>>4)&3,	top,  area->height - area->deltaHeight, top, bottom);
	nbottom = resizeEdge(p->y + p->height,	(autosizeMask>>6)&3,    top,  area->height - area->deltaHeight, top, bottom);
	p->x = nleft;
	p->y = ntop;
	p->width = nright - nleft;
	p->height = nbottom - ntop;
}

gkPanelLayoutMethod gkLayoutMethodAutosize(uint16_t mask)
{
    gkPanelLayoutMethod method = {
        gkLayoutFuncAutosize,
        (int32_t)mask, 0,0,0,0
    };
    return method;
}

/* Advanced layout method*/

void gkLayoutFuncAdvanced(gkPanel* p, gkClientArea* area)
{
    gkAdvancedLayoutParams* params = (gkAdvancedLayoutParams*)&p->layoutMethod.params;
    float w = area->width - (params->margin.left + params->margin.right);
    float h = area->height - (params->margin.top + params->margin.bottom);
    if(params->flags & GK_RELATIVE_X)
        p->x = params->margin.left + w*params->relativeX;
    if(params->flags & GK_RELATIVE_Y)
        p->y = params->margin.top + h*params->relativeY;
    if(params->flags & GK_RELATIVE_WIDTH)
        p->width = w*params->relativeWidth;
    if(params->flags & GK_RELATIVE_HEIGHT)
        p->height = h*params->relativeHeight;
    if((params->flags & GK_LAYOUT_MIN_WIDTH) &&  p->width < params->minWidth)
        p->width = params->minWidth;
    if((params->flags & GK_LAYOUT_MAX_WIDTH) &&  p->width > params->maxWidth)
        p->width = params->maxWidth;
    if((params->flags & GK_LAYOUT_MIN_HEIGHT) &&  p->height < params->minHeight)
        p->height = params->minHeight;
    if((params->flags & GK_LAYOUT_MAX_HEIGHT) &&  p->height > params->maxHeight)
        p->height = params->maxHeight;
}

gkPanelLayoutMethod gkLayoutMethodAdvanced(gkAdvancedLayoutParams* params)
{
    gkPanelLayoutMethod method;
    method.func = gkLayoutFuncAdvanced;
    memcpy(&method.params, params, sizeof(gkAdvancedLayoutParams));
    return method;
}

gkPanelLayoutMethod gkLayoutMethodNone()
{
    gkPanelLayoutMethod method = {
        0,
        0,0,0,0,0
    };
    return method;
}

gkPanel* gkFocusPanel = 0;

gkPanel* gkCreatePanel()
{
    return gkCreatePanelEx(sizeof(gkPanel));
}

gkPanel* gkCreatePanelEx(size_t panelSize){
	gkPanel* panel = (gkPanel*)malloc(panelSize);
	gkInitListenerList(&panel->listeners);
	panel->x = 0;
	panel->y = 0;
	panel->width = 0;
	panel->height = 0;
	panel->transform = gkMatrixCreateIdentity();
	panel->anchorX = 0;
	panel->anchorY = 0;
	panel->colorFilter = GK_COLOR(1,1,1,1);
	panel->data = 0;
	panel->layoutMethod = gkLayoutMethodNone();
	panel->updateFunc = 0;
	panel->drawFunc = 0;
	panel->parent = 0;
	panel->numChildren = 0;
	panel->mouseOver = GK_FALSE;
	panel->mouseX = panel->mouseY = 0;
	panel->mouseEnabled = panel->mouseChildren = panel->keyboardEnabled = panel->keyboardChildren = GK_TRUE;
	panel->visible = GK_TRUE;

	panel->mChildren.first = panel->mChildren.last = 0;
	panel->mNext = 0;
	panel->mNextChild = 0;
	panel->mGuardDestroy = GK_FALSE;
	panel->mMustDestroy = GK_FALSE;
	panel->mViewport = GK_FALSE;
	panel->mClientArea.x = 0;
	panel->mClientArea.y = 0;
	panel->mClientArea.width = 0;
	panel->mClientArea.height = 0;
	panel->mClientArea.deltaX = panel->mClientArea.deltaY =
        panel->mClientArea.deltaWidth = panel->mClientArea.deltaHeight = 0;
	return panel;
}

gkPanel* gkCreateViewportPanel()
{
    return gkCreateViewportPanelEx(sizeof(gkPanel));
}

gkPanel* gkCreateViewportPanelEx(size_t panelSize){
	gkPanel* panel = gkCreatePanelEx(panelSize);
	panel->mViewport = GK_TRUE;
	return panel;
}

static void gkGuardDestroy(gkPanel* panel)
{
    panel->mGuardDestroy = GK_TRUE;
}
static void gkUnguardDestroy(gkPanel* panel)
{
    panel->mGuardDestroy = GK_FALSE;
    if(panel->mMustDestroy)
        gkDestroyPanel(panel);
}

void gkDestroyPanel(gkPanel* panel){
	gkPanel *p;
	if(panel->mGuardDestroy){
		panel->mMustDestroy = GK_TRUE;
	}else{
		if(panel->parent) gkRemoveChild(panel);
		if(gkFocusPanel == panel) gkSetFocus(0);
		gkCleanupListenerList(&panel->listeners);
		for(p = panel->mChildren.first; p; p = panel->mNextChild){
			panel->mNextChild = p->mNext;
			p->parent = 0;
			p->mNext = 0;
		}
		free(panel);
	}
}

void gkAddChild(gkPanel* parent, gkPanel* child){
    gkAddChildAt(parent, child, parent->numChildren);
}

void gkAddChildAt(gkPanel* parent, gkPanel* child, int index){
    gkPanel* p;
	gkEvent evt;
	int i = 1;
	if(child->parent) gkRemoveChild(child);

	gkUpdateLayout(parent);

	child->parent = parent;
	child->mNext = 0;
	if(parent->mChildren.last){
        if(index >= parent->numChildren)
        {
            parent->mChildren.last->mNext = child;
            parent->mChildren.last = child;
        }else if(index>0){
			for(p = parent->mChildren.first; p && i<index; p = p->mNext) i++;
			child->mNext = p->mNext;
			p->mNext = child;
		}else{
			child->mNext = parent->mChildren.first;
			parent->mChildren.first = child;
		}
	}else{
		parent->mChildren.first = parent->mChildren.last = child;
	}
	parent->numChildren++;

	evt.type = GK_ON_PANEL_ADDED;
	evt.target = evt.currentTarget = child;
	gkDispatch(child, &evt);
}

void gkRemoveChild(gkPanel* child){
	gkPanel *panel = child->parent;
	if(panel){
		gkEvent evt;
		gkPanel* p, *prev = 0;
		for(p = panel->mChildren.first; p && p != child; p = p->mNext) prev = p;
		if(p != 0){
			if(p == panel->mChildren.last) panel->mChildren.last = prev;
			if(prev){
				prev->mNext = p->mNext;
			}else{
				panel->mChildren.first = p->mNext;
			}
			if(p == panel->mNextChild) panel->mNextChild = p->mNext;
			panel->numChildren--;
		}
		child->parent = 0;
		child->mNext = 0;
		if(gkFocusPanel == child) gkSetFocus(0);
		evt.type = GK_ON_PANEL_REMOVED;
		evt.target = evt.currentTarget = child;
		gkDispatch(child, &evt);
	}
}

void gkRemoveChildAt(gkPanel* parent, int childIndex){
	gkRemoveChild(gkGetChildAt(parent, childIndex));
}

int gkGetChildIndex(gkPanel* child){
	gkPanel *parent = child->parent, *p;
	int index = 0;
	if(parent){
		for(p = parent->mChildren.first; p && p != child; p = p->mNext)		index++;
	}
	return index;
}

gkPanel* gkGetChildAt(gkPanel* parent, int childIndex){
	gkPanel *p;
	int index = 0;
	if(parent){
		for(p = parent->mChildren.first; p && index != childIndex; p = p->mNext)		index++;
	}
	return p;
}

void gkProcessChildrenLayout(gkPanel* panel)
{
    gkPanel* p;
	gkGuardDestroy(panel);
	for(p = panel->mChildren.first; p; p = panel->mNextChild){
		panel->mNextChild = p->mNext;
		gkProcessLayoutPanel(p, &panel->mClientArea);
	}
	gkUnguardDestroy(panel);
}

void gkUpdateLayout(gkPanel* panel)
{
    gkClientArea old = panel->mClientArea;
	gkUpdateClientArea(&panel->mClientArea, &old, panel->x, panel->y, panel->width, panel->height);
	gkProcessChildrenLayout(panel);
}

void gkProcessLayoutPanel(gkPanel* panel, gkClientArea* clientArea)
{
    gkClientArea old = panel->mClientArea;
	gkUpdateClientArea(&panel->mClientArea, &old, panel->x, panel->y, panel->width, panel->height);

	if(panel->layoutMethod.func)
    {
		panel->layoutMethod.func(panel, clientArea);
        gkUpdateClientArea(&panel->mClientArea, &old, panel->x, panel->y, panel->width, panel->height);
	}

    gkProcessChildrenLayout(panel);
}

void gkProcessLayoutMainPanel(gkPanel* panel, float width, float height)
{
	gkUpdateClientArea(&panel->mClientArea, &panel->mClientArea, panel->x, panel->y, width, height);

	if(panel->layoutMethod.func)
    {
		panel->layoutMethod.func(panel, &panel->mClientArea);
    }

	panel->width = width;
	panel->height = height;

    gkProcessChildrenLayout(panel);
}

void gkProcessUpdatePanel(gkPanel* panel){
	gkPanel* p;
	gkGuardDestroy(panel);
	if(panel->updateFunc){
		panel->updateFunc(panel);
	}
	for(p = panel->mChildren.first; p; p = panel->mNextChild){
		panel->mNextChild = p->mNext;
		gkProcessUpdatePanel(p);
	}
	gkUnguardDestroy(panel);
}

void gkProcessDrawPanel(gkPanel* panel)
{
	gkPanel* p;
	gkMatrix t = gkMatrixCreateTranslation(panel->x, panel->y);
	gkMatrix t2 = gkMatrixCreateTranslation(-panel->anchorX*panel->width, -panel->anchorY*panel->height);
	if(!panel->visible)
        return;	/* Don't draw invisible panels */
	gkGuardDestroy(panel);
	gkPushColorFilter(panel->colorFilter.r, panel->colorFilter.g, panel->colorFilter.b, panel->colorFilter.a);
	gkPushTransform(&t);
	gkPushTransform(&panel->transform);
	gkPushTransform(&t2);
	if(panel->mViewport)
    {
		gkMatrix m = gkLocalToGlobal(panel);
		gkPoint topLeft = gkTransformPoint(GK_POINT(0,0), &m);
		gkPoint bottomRight = gkTransformPoint(GK_POINT(panel->width, panel->height), &m);
		float h = FABS(bottomRight.y - topLeft.y);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
		glViewport((GLint)topLeft.x, (GLint)gkGetScreenSize().height - topLeft.y - h, FABS(bottomRight.x - topLeft.x), h);
		if(panel->drawFunc){
			panel->drawFunc(panel);
		}
		glPopClientAttrib();
		glPopAttrib();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}else{
		if(panel->drawFunc){
			panel->drawFunc(panel);
		}
		for(p = panel->mChildren.first; p; p = panel->mNextChild){
			panel->mNextChild = p->mNext;
			gkProcessDrawPanel(p);
		}
	}
	gkPopTransform();
	gkPopTransform();
	gkPopTransform();
	gkPopColorFilter();
	gkUnguardDestroy(panel);
}

gkMatrix gkGlobalToLocal(gkPanel* panel){
	gkMatrix m = gkLocalToGlobal(panel);
	gkMatrixInverse(&m);
	return m;
}

gkMatrix gkLocalToGlobal(gkPanel* panel){
	gkMatrix m = gkMatrixCreateIdentity(), t, t2;
	gkPanel* p = panel;
	do{
		t = gkMatrixCreateTranslation(p->x, p->y);
		t2 = gkMatrixCreateTranslation(-p->anchorX*p->width, -p->anchorY*p->height);
		gkMatrixMultPtr(&m, &t2);
		gkMatrixMultPtr(&m, &p->transform);
		gkMatrixMultPtr(&m, &t);
		p = p->parent;
	}while(p);
	return m;
}

/*** Panel input handling ***/

gkPanel* gkMouseTarget = 0;
gkPoint gkMousePosition;

#define BYTE_OFFSET(obj, prop) ((char*)&obj->prop - (char*)obj)

gkPanel* gkGetMouseTarget(gkPanel* panel, gkPoint pos, size_t enabledOffset, size_t enabledChildrenOffset){
	gkPanel** children, *p;
	int i = panel->numChildren;
	GK_BOOL *enabled = (GK_BOOL*)((char*)panel + enabledOffset);
	GK_BOOL *enabledChildren = (GK_BOOL*)((char*)panel + enabledChildrenOffset);
	if(!panel->visible) return 0;	/* ignore invisible panels */
	if(*enabledChildren){
		children = (gkPanel**)calloc(panel->numChildren, sizeof(gkPanel*));
		for(p = panel->mChildren.first; p && i>0; p = p->mNext) children[--i] = p;	/* also reverses the order so in the next loop it starts from the last and goes to first */
		for(i = 0; i<panel->numChildren; i++){
            gkMatrix translate, t;
			gkPoint tmpPos = pos;
            float det;
			p = children[i];
            translate = gkMatrixCreateTranslation(p->x, p->y);
            t = gkMatrixCreateTranslation(-p->anchorX*p->width, -p->anchorY*p->height);
            gkMatrixMultPtr(&t, &p->transform);
			gkMatrixMultPtr(&t, &translate);
			gkMatrixInverse(&t);
			tmpPos = gkTransformPoint(tmpPos, &t);
			if((p = gkGetMouseTarget(p, tmpPos, enabledOffset, enabledChildrenOffset)) != 0){
				panel->mouseX = pos.x;
				panel->mouseY = pos.y;
				free(children);
				return p;
			}
		}
		free(children);
	}
	if(pos.x>=0 && pos.y>=0 && pos.x<=panel->width && pos.y<=panel->height && *enabled){
		panel->mouseX = pos.x;
		panel->mouseY = pos.y;
		return panel;
	}else return 0;
}

void gkCheckFocusedPanel(){
	if(gkFocusPanel){
		gkPanel* p = gkFocusPanel;
		GK_BOOL focusable = p->keyboardEnabled && p->visible;
		if(focusable){
			p = p->parent;
			while(p && focusable){
				if(!(p->keyboardChildren && p->visible)) focusable = GK_FALSE;
				p = p->parent;
			}
		}
		if(!focusable) gkSetFocus(0);
	}
}

void gkUpdateMouseTarget(gkPanel* mainPanel){
	gkPanel *oldMouseTarget, *p, *lastCommon;
	gkPoint pos = gkMousePosition;
	if(!mainPanel){
		gkMouseTarget = 0;
		return;
	}
	oldMouseTarget = gkMouseTarget;
	gkMouseTarget = gkGetMouseTarget(mainPanel, pos, BYTE_OFFSET(mainPanel, mouseEnabled), BYTE_OFFSET(mainPanel, mouseChildren));
	if(oldMouseTarget != gkMouseTarget){
		gkMouseEvent enter, leave;
		enter.type = GK_ON_MOUSE_ENTER;
		enter.target = gkMouseTarget;
		enter.position = gkMousePosition;
		leave.type = GK_ON_MOUSE_LEAVE;
		leave.target = oldMouseTarget;
		leave.position = gkMousePosition;

		for(p = oldMouseTarget; p; p = p->parent) p->mouseOver = GK_FALSE;	/* reset mouse over */
		for(p = gkMouseTarget; p; p = p->parent) p->mouseOver = GK_TRUE;	/* set mouse over to all panels under the mouse */
		for(p = oldMouseTarget; p && !p->mouseOver; p = p->parent){
			leave.currentTarget = p;
			gkDispatch(p, &leave);	/* dispatch GK_ON_MOUSE_LEAVE */
		}
		lastCommon = p;
		for(p = gkMouseTarget; p != 0 && p != lastCommon; p = p->parent){
			enter.currentTarget = p;
			gkDispatch(p, &enter);	/* dispatch GK_ON_MOUSE_ENTER */
		}
	}
}

void gkProcessMouseEvent(gkMouseEvent* mouseEvent){
	gkPanel* current, *newFocusTarget = 0, *curFocusTarget = gkFocusPanel;
	gkMousePosition = mouseEvent->position;
	if(mouseEvent->type == GK_ON_MOUSE_DOWN){
		gkPanel* mainPanel = gkGetMainPanel();
		if(mainPanel){
			newFocusTarget = gkGetMouseTarget(mainPanel, gkMousePosition, BYTE_OFFSET(mainPanel, keyboardEnabled), BYTE_OFFSET(mainPanel, keyboardChildren));
		}
	}
	if(gkMouseTarget){
		mouseEvent->target = current = gkMouseTarget;
		do{
			gkMatrix m = gkGlobalToLocal(current);
			mouseEvent->position = gkTransformPoint(gkMousePosition, &m);
			mouseEvent->currentTarget = current;
		}while(gkDispatch(current, mouseEvent) && (current = current->parent));
	}
	if(newFocusTarget && curFocusTarget == gkFocusPanel){
		gkSetFocus(newFocusTarget);
	}
}


void gkSetFocus(gkPanel* panel){
	static gkPanel *tmpNextFocused = 0;
	gkPanel* tmp = gkFocusPanel;
	if(panel == gkFocusPanel) return;
	tmpNextFocused = panel;
	gkFocusPanel = 0;
	if(tmp){
		gkEvent evt;
		evt.type = GK_ON_PANEL_FOCUS_OUT;
		evt.target = evt.currentTarget = tmp;
		gkDispatch(tmp, &evt);
	}
	if(gkFocusPanel != tmpNextFocused){
		if(tmpNextFocused->keyboardEnabled && tmpNextFocused->visible){
			gkFocusPanel = tmpNextFocused;
			if(gkFocusPanel){
				gkEvent evt;
				evt.type = GK_ON_PANEL_FOCUS_IN;
				evt.target = evt.currentTarget = gkFocusPanel;
				gkDispatch(gkFocusPanel, &evt);
			}
		}
	}
}

gkPanel* gkGetFocus(){
	return gkFocusPanel;
}

void gkProcessKeyboardEvent(gkKeyboardEvent* keyboardEvent){
	if(!gkFocusPanel) return;
	keyboardEvent->target = keyboardEvent->currentTarget = gkFocusPanel;
	gkDispatch(gkFocusPanel, keyboardEvent);
}
