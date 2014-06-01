/* Copyright (c) 2014 Toni Georgiev
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


#include <gk.h>
#include "gkImageInternal.h"
#include "gkGL.h"

#include <stdio.h>
#include <stdlib.h>


#include <locale.h>

gkColor gkGetFilteredColor(gkColor c);	//implemented in graphics.c

void gkInitImages(){
}

void gkCleanupImages(){
}

gkImage* gkLoadImage(char* filename)
{
	GLint oldTexId;
	gkImage* image;
	gkImageData* imageData = gkDecodeImage(filename);

	if (!imageData)
		return 0;

	image = (gkImage*)malloc(sizeof(gkImage));
	image->width = imageData->width;
	image->height = imageData->height;

	glGenTextures(1, &image->id);

	glEnable(GL_TEXTURE_2D);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexId);
	glBindTexture(GL_TEXTURE_2D, image->id);

	if (imageData->pixelFormat == GK_PIXELFORMAT_RGBA) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 
			0, GL_RGBA, GL_UNSIGNED_BYTE, imageData->data);
	} else if (imageData->pixelFormat == GK_PIXELFORMAT_RGB) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 
			0, GL_RGB, GL_UNSIGNED_BYTE, imageData->data);
	}

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glBindTexture(GL_TEXTURE_2D, oldTexId);
	glDisable(GL_TEXTURE_2D);

	return image;
}

GK_BOOL gkSaveImage(gkImage* image, char* filename)
{
	GK_BOOL result;
	gkImageData *imageData = gkCreateImageData(image->width, image->height, GK_PIXELFORMAT_RGBA);
	gkGetImageData(image, GK_PIXELFORMAT_RGBA, imageData->data);
	result = gkEncodeImage(filename, imageData);
	gkDestroyImageData(imageData);
	return result;
}

gkImage* gkCreateImage(int width, int height){
	gkImage* image = (gkImage*)malloc(sizeof(gkImage));
	GLint oldTexId;

	glGenTextures(1, &image->id);
	image->width = width;
	image->height = height;

	glEnable(GL_TEXTURE_2D);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexId);
	glBindTexture(GL_TEXTURE_2D, image->id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glBindTexture(GL_TEXTURE_2D, oldTexId);
	glDisable(GL_TEXTURE_2D);

	return image;
}

void gkSetImageData(gkImage* image, gkPixelFormat format, void* data)
{
	GLint oldId;
	GLint oldUnPackAlignment;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnPackAlignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glEnable(GL_TEXTURE_2D);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldId);
	glBindTexture(GL_TEXTURE_2D, image->id);

	if (format == GK_PIXELFORMAT_RGBA) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height,0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	} else if (format == GK_PIXELFORMAT_RGB) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height,0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}

	glBindTexture(GL_TEXTURE_2D, oldId);
	glDisable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnPackAlignment);
}

void gkGetImageData(gkImage* image, gkPixelFormat format, void* data)
{
	GLint oldId;
	GLint oldPackAlignment;
	GLuint fmt;
	glGetIntegerv(GL_PACK_ALIGNMENT, &oldPackAlignment);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldId);
	glBindTexture(GL_TEXTURE_2D, image->id);

	if (format == GK_PIXELFORMAT_RGBA) {
		fmt = GL_RGBA;
	}else if (format == GK_PIXELFORMAT_RGB) {
		fmt = GL_RGB;
	}

#if 1
	/* If glGetTexImage is available */
	glGetTexImage(GL_TEXTURE_2D, 0, fmt, GL_UNSIGNED_BYTE, data);
#else 
	if (gkBeginDrawToImage(image, GK_FALSE)) {
		glFlush();
		glReadPixels(0,0, image->width, image->height, 
			fmt, GL_UNSIGNED_BYTE, data);
		gkEndDrawToImage();
	}
#endif

	glBindTexture(GL_TEXTURE_2D, oldId);
	glPixelStorei(GL_PACK_ALIGNMENT, oldPackAlignment);
}

void gkDrawImageInternal(gkImage* img, float* v, float* c){
	gkColor color = gkGetFilteredColor(GK_COLOR(1,1,1,1));
	glColor4f(color.r, color.g, color.b, color.a);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, img->id);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, v);
	glTexCoordPointer(2, GL_FLOAT, 0, c);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void gkDrawImage(gkImage* image, float x, float y){
	float v[]= {
		0,0,
		image->width, 0,
		image->width, image->height,
		0, image->height
	};
	float c[]= {
		0, 0,
		1, 0,
		1, 1,
		0, 1
	};

	glPushMatrix();
	glTranslatef(x,y,0);
	gkDrawImageInternal(image, v, c);
	glPopMatrix();
}

void gkDrawImageEx(gkImage* image, float x, float y, gkRect srcRect){
	float v[]= {
		0,0,
		srcRect.width, 0,
		srcRect.width, srcRect.height,
		0, srcRect.height
	};
	float left = (srcRect.x/image->width);
	float top = (srcRect.y/image->height);
	float right = left + (srcRect.width/image->width);
	float bottom = top + (srcRect.height/image->height);

	float c[]= {
		left, top,
		right, top,
		right, bottom,
		left, bottom
	};

	glPushMatrix();
	glTranslatef(x,y,0);
	gkDrawImageInternal(image, v, c);
	glPopMatrix();
}


void gkDestroyImage(gkImage* image){
	glDeleteTextures(1, &image->id);
	free(image);
}