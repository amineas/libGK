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

#ifndef _GK_GEOMETRY_H_
#define _GK_GEOMETRY_H_

/*
 *	Geometry
 *
 *	Some useful geometric structures and functions
 */

typedef struct gkPoint
{
	float x,y;
}gkPoint;

typedef struct gkSize
{
	float width, height;
}gkSize;

typedef struct gkRect
{
	float x,y,width,height;
}gkRect;

typedef struct gkMatrix
{
	float data[9];
}gkMatrix;

gkPoint	GK_POINT(float x, float y);
gkSize	GK_SIZE(float width, float height);
gkRect	GK_RECT(float x, float y, float width, float height);

void	gkMatrixMult(gkMatrix* dst, gkMatrix src);
void	gkMatrixMultPtr(gkMatrix* dst, gkMatrix* src);
float	gkMatrixDeterminant(gkMatrix* dst);
void	gkMatrixInverse(gkMatrix* dst);
void	gkMatrixTranspose(gkMatrix* dst);

gkMatrix gkMatrixCreateIdentity();
gkMatrix gkMatrixCreateTranslation(float x, float y);
gkMatrix gkMatrixCreateRotation(float radians);
gkMatrix gkMatrixCreateScale(float sx, float sy);

gkPoint gkTransformPoint(gkPoint point, gkMatrix* matrix);	//performs Point * Matrix multiplication


#endif