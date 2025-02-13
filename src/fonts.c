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

#define GK_INTERNAL

#include "gk.h"
#include "gk_internal.h"

#ifdef GK_USE_FONTS

#include "gkGL.h"
#include "gkStream.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H

FT_Library ftlib;

#define GK_FONT_TOTAL_STYLES 4

#define CHAR(x)	((uint32_t)x)

gkTextFormat gkDefaultTextFormat = {
	GK_TEXT_ALIGN_LEFT,		/*	align */
	GK_TEXT_VALIGN_TOP,		/*	valign */
	GK_FALSE,			/*	wordWrap */
	4,				/*	tab space */
	GK_FALSE,			/*	underline */
	0,				/*	width */
	0,				/*	height	*/
	0,				/*	stroke size	*/
	0,				/*	line spacing */
	{1,1,1,1},			/*	text color	*/
	{0,0,0,1},			/*	stroke color */
	GK_FALSE,			/*	vertical */
	GK_TRUE				/*  keep baseline */
};

typedef struct gkBBoxStruct gkBBox;
struct gkBBoxStruct{
	float minX;
	float minY;
	float maxX;
	float maxY;
};

gkBBox gkCreateBBox(float minx, float miny, float maxx, float maxy)
{
	gkBBox r = {minx, miny, maxx, maxy};
	return r;
}

void gkBBoxAdd(gkBBox* dst, gkBBox *src)
{
	if(dst->minX > src->minX) dst->minX = src->minX;
	if(dst->minY > src->minY) dst->minY = src->minY;
	if(dst->maxX < src->maxX) dst->maxX = src->maxX;
	if(dst->maxY < src->maxY) dst->maxY = src->maxY;
}

void gkBBoxTranslate(gkBBox* dst, float tx, float ty)
{
	dst->minX += tx;
	dst->maxX += tx;
	dst->minY += ty;
	dst->maxY += ty;
}

typedef struct gkGlyphStruct gkGlyph;
struct gkGlyphStruct{
	int index;
	gkSize size;
	gkPoint offset;
	gkPoint advance;
	gkBBox bbox;
	uint32_t texId;
	gkRect texCoords;
};

typedef struct gkGlyphSetStruct gkGlyphSet;
struct gkGlyphSetStruct{
	uint32_t texId;
	gkGlyph** glyphs;
};

typedef struct gkGlyphCollectionStruct gkGlyphCollection;
struct gkGlyphCollectionStruct{
	uint16_t size;
	float strokeSize;
	uint16_t setBits;
	uint16_t texWidth;
	uint16_t texHeight;
	uint16_t cellWidth;
	uint16_t cellHeight;
	uint16_t glyphSetCount;
	gkPoint spaceAdvance;
	gkGlyphSet** glyphSets;
	gkGlyphCollection* next;
};

struct gkFontFaceStructEx{
	char* fontFamily;
	uint8_t style;
	/*Extended info*/
	FT_Face ftface;
	gkGlyphCollection* collections;
};
typedef struct gkFontFaceStructEx gkFontFaceEx;

typedef struct gkFontRcRefStruct gkFontRcRef;
struct gkFontRcRefStruct{
	gkFontResource* resource;
	gkFontRcRef* next;
};
gkFontRcRef* gkFontResources, *gkFontResourcesTop;

void gkInitFont(gkFont* font);
void gkDestroyFace(gkFontFaceEx* face);

gkGlyphCollection* gkGetGlyphCollection(gkFont* font, float strokeSize);
gkGlyphSet* gkGetGlyphSet(gkGlyphCollection* collection, int index);
gkGlyph* gkGetGlyph(FT_Face face, gkGlyphCollection* collection, gkGlyphSet* set, uint32_t index);

void gkDestroyGlyphCollection(gkGlyphCollection* collection);
void gkDestroyGlyphSet(gkGlyphSet* set, int setSize);
void gkDestroyGlyph(gkGlyph* glyph);

/* functions */

void gkInitFonts()
{
	FT_Error err = FT_Init_FreeType(&ftlib);
	if (err) {
		printf("GK [ERROR]: FreeType2 could not be initializeed.\n");
		gkExit();
	}
	gkFontResources = 0;
	gkFontResourcesTop = 0;
}

static void printFontResourceError(FT_Error error, char* filename)
{
	if (error == FT_Err_Unknown_File_Format) {
		printf("GK [ERROR]: Unknown File Format %s\n", filename);
	} else if (error == FT_Err_Cannot_Open_Resource) {
		printf("GK [ERROR]: Cannot Open Resource %s\n", filename);
	} else if (error == FT_Err_Invalid_File_Format) {
		printf("GK [ERROR]: Invalid File Format %s\n", filename);
	} else if (error == FT_Err_Unimplemented_Feature) {
		printf("GK [ERROR]: Unimplemented Feature %s\n", filename);
	} else if (error == FT_Err_Missing_Module) {
		printf("GK [ERROR]: Missing Module %s\n", filename);
	} else if (error) {
		printf("GK [ERROR]: Could not load font resource %s\n", filename);
	}
}

static unsigned long readFTStream(FT_Stream ftStream, unsigned long offset, 
				  unsigned char* buffer, unsigned long count)
{
	gkStream* stream = (gkStream*)ftStream->descriptor.pointer;
	int res = gkStreamSeek(stream, offset, GK_SEEK_SET);
	if (count>0)
		return gkStreamRead(stream, buffer, count);
	return res;
}

static void closeFTStream(FT_Stream ftStream)
{
	gkStream* stream = (gkStream*)ftStream->descriptor.pointer;
	gkStreamClose(stream);
	free(ftStream);
}

static gkStream* openStream(char* filename)
{
#if defined(GK_PLATFORM_ANDROID)
	void *dat;
	size_t datSize;

	if (gkReadFile(filename, &dat, &datSize))
		return gkOpenMemory(dat, datSize, GK_TRUE);
	
	return 0;
#else
	return gkOpenFile(filename, "rb");
#endif
}

static FT_Stream makeFTStream(char* filename)
{
	gkStream* stream = openStream(filename);
	FT_Stream ftStream;
	if (!stream)
		return 0;

	ftStream = (FT_Stream)malloc(sizeof(FT_StreamRec));
	memset(ftStream, 0, sizeof(FT_StreamRec));

	gkStreamSeek(stream, 0, GK_SEEK_END);
	ftStream->size = gkStreamTell(stream);
	gkStreamSeek(stream, 0, GK_SEEK_SET);

	ftStream->descriptor.pointer = stream;
	ftStream->read = readFTStream;
	ftStream->close = closeFTStream;
	return ftStream;
}

static FT_Error openFace(char* filename, FT_Long index, FT_Face* aface)
{
	FT_Open_Args args = {
		FT_OPEN_STREAM,
		0,0,	/* memory_base, memory_size */
		0,	/* filename */
		makeFTStream(filename),	/* stream */
		0,	/* driver */
		0,	/* extra params */
		0	/* params */
	};
	return FT_Open_Face(ftlib, &args, index, aface);
}

gkFontResource* gkAddFontResource(char* filename)
{
	FT_Face face;
	gkFontResource *resource = 0;
	gkFontRcRef* ref;
	gkFontFaceEx* f;
	int i = 0;
	FT_Error error;
	do {
//		error = FT_New_Face(ftlib, filename, i, &face);
		error = openFace(filename, i, &face);
		if (error) {
			printFontResourceError(error, filename);
			return 0;
		}
		if (resource == 0) {
			resource = (gkFontResource*)malloc(sizeof(gkFontResource));
			resource->numFaces = (uint8_t)face->num_faces;
			resource->faces = (gkFontFace**)calloc(face->num_faces, sizeof(gkFontFaceEx*));
			ref = (gkFontRcRef*)malloc(sizeof(gkFontRcRef));
			ref->resource = resource;
			ref->next = 0;
			if (gkFontResources) {
				gkFontResourcesTop->next = ref;
				gkFontResourcesTop = ref;
			} else {
				gkFontResources = gkFontResourcesTop = ref;
			}
		}
		f = (gkFontFaceEx*)(resource->faces[i] = (gkFontFace*)malloc(sizeof(gkFontFaceEx)));
		f->fontFamily = face->family_name;
		f->style = (uint8_t)face->style_flags;
		f->ftface = face;
		f->collections = 0;
		i++;
	} while (i < resource->numFaces);
	return resource;
}

void gkRemoveFontResource(gkFontResource* rc)
{
	gkFontRcRef* ref = gkFontResources, *prev = 0, *p;
	gkFontFaceEx* face;
	int i;
	while (ref) {
		if (ref->resource == rc) {
			p = ref;
			for (i = 0; i < rc->numFaces; i++) {
				face = (gkFontFaceEx*)rc->faces[i];
				gkDestroyFace(face);
			}
			free(rc->faces);
			free(p->resource);
			if (prev) {
				prev->next = p->next;
			} else {
				gkFontResources = p->next;
			}
			ref = ref->next;
			free(p);
		} else {
			prev = ref;
			ref = ref->next;
		}
	}
}

gkFont* gkCreateFont(char* family, uint16_t size, uint8_t style)
{
	gkFont* font;
	gkFontRcRef* p = gkFontResources;
	gkFontResource* resource;
	gkFontFaceEx* face;
	int i;
	while (p) {
		resource = p->resource;
		for (i = 0; i < resource->numFaces; i++) {
			face = (gkFontFaceEx*)resource->faces[i];
			if (stricmp(face->fontFamily, family) == 0 && face->style == style) {
				font = (gkFont*)malloc(sizeof(gkFont));
				font->face = (gkFontFace*)face;
				font->size = size;
				gkInitFont(font);
				return font;
			}
		}
		p = p->next;
	}
	return 0;
}

void gkDestroyFont(gkFont* font)
{
	free(font);
}

static void cleanupBatch();

void gkCleanupFonts()
{
	gkFontRcRef* ref = gkFontResources, *p;
	gkFontFaceEx* face;
	int i;
	while (ref) {
		p = ref;
		for (i = 0; i < p->resource->numFaces; i++) {
			face = (gkFontFaceEx*)p->resource->faces[i];
			FT_Done_Face(face->ftface);
			free(face);
		}
		free(p->resource->faces);
		free(p->resource);
		ref = ref->next;
		free(p);
	}
	gkFontResources = gkFontResourcesTop = 0;
	FT_Done_FreeType(ftlib);
	ftlib = 0;
	cleanupBatch();
}

void gkInitFont(gkFont* font)
{
}


/*
	Glyph caching
*/

/* 
	This is BAD!
	Should detect MIN & MAX texture size on the actual device.
*/
#ifdef GK_PLATFORM_ANDROID
#define GK_MIN_FONT_TEX_SIZE	64
#define GK_MAX_FONT_TEX_SIZE	512
#else
#define GK_MIN_FONT_TEX_SIZE	64
#define GK_MAX_FONT_TEX_SIZE	1024
#endif

void gkInitGlyphCollection(gkGlyphCollection* collection, gkFont* font, float strokeSize);
GK_BOOL gkTestCollection(gkGlyphCollection* collection, int glyphW, int glyphH);
gkGlyph* gkMakeGlyph(FT_Face face, gkGlyphCollection* collection, gkGlyphSet* glyphSet, int index);
void gkGetCellPos(gkGlyphCollection* collection, int index, int *x, int *y);

gkGlyph* gkGetGlyph(FT_Face face, gkGlyphCollection* collection, gkGlyphSet* glyphSet, uint32_t index){
	int glyphIndex = index&(0xFF>>(8 - collection->setBits));
	gkGlyph* glyph = glyphSet->glyphs[glyphIndex];
	if(!glyph){
		glyph = glyphSet->glyphs[glyphIndex] = gkMakeGlyph(face, collection, glyphSet, index);
	}
	return glyph;
}


gkGlyphCollection* gkGetGlyphCollection(gkFont* font, float strokeSize){
	gkFontFaceEx* face = (gkFontFaceEx*)font->face;
	gkGlyphCollection* p = face->collections;
	if(p){
		while(p){
			if(p->size == font->size && p->strokeSize == strokeSize) return p;
			p = p->next;
		}
	}
	p = (gkGlyphCollection*)malloc(sizeof(gkGlyphCollection));
	gkInitGlyphCollection(p, (gkFont*)font, strokeSize);
	if(face->collections){
		p->next = face->collections;
	}
	face->collections = p;
	return p;
}

void gkInitGlyphCollection(gkGlyphCollection* collection, gkFont* font, float strokeSize){
	int Gw, Gh;
	FT_Face face = ((gkFontFaceEx*)font->face)->ftface;
	Gw = FT_MulFix(face->bbox.xMax - face->bbox.xMin, face->size->metrics.x_scale)/64 + (int)strokeSize;
	Gh = FT_MulFix(face->bbox.yMax - face->bbox.yMin, face->size->metrics.y_scale)/64 + (int)strokeSize;
	collection->glyphSets = 0;
	collection->glyphSetCount = 0;
	collection->size = font->size;
	collection->strokeSize = strokeSize;
	collection->next = 0;
	collection->setBits = 8;
	collection->texWidth = GK_MIN_FONT_TEX_SIZE;
	collection->texHeight = GK_MIN_FONT_TEX_SIZE;
	collection->cellWidth = Gw;
	collection->cellHeight = Gh;

	FT_Load_Char(face, L' ', FT_LOAD_DEFAULT);
	collection->spaceAdvance.x = ((float)face->glyph->advance.x)/64.0f;
	collection->spaceAdvance.y = ((float)face->glyph->advance.y)/64.0f;
	if(collection->spaceAdvance.y == 0)
		collection->spaceAdvance.y = collection->spaceAdvance.x;

	while(!gkTestCollection(collection, Gw, Gh)){
		if (collection->texWidth < GK_MAX_FONT_TEX_SIZE) {
			collection->texWidth <<=1;
		}else if (collection->texHeight < GK_MAX_FONT_TEX_SIZE) {
			collection->texWidth = GK_MIN_FONT_TEX_SIZE;
			collection->texHeight <<=1;
		}else if (collection->setBits > 0) {
			collection->setBits--;
			collection->texWidth = collection->texHeight = GK_MIN_FONT_TEX_SIZE;
		}else{
			printf("GK [ERROR]: font error\n");
			break;
		}
	}
/*	printf("GK [VERBOSE]: new collection W: %d H: %d Set: %d \ngw: %d gh: %d\n", 
		collection->texWidth, collection->texHeight, 
		((0xFF>>(8 - collection->setBits)) + 1),
		Gw, Gh);*/
}

GK_BOOL gkTestCollection(gkGlyphCollection* collection, int glyphW, int glyphH){
	int setSize = (0xFF>>(8 - collection->setBits)) + 1;
	int glyphsPerRow = collection->texWidth / glyphW;
	if(glyphsPerRow>0){
		int rows = setSize / glyphsPerRow;
		if ((setSize % glyphsPerRow) > 0)
			rows++;
		return rows * glyphH <= collection->texHeight;
	}else{
		return GK_FALSE;
	}
}

gkGlyphSet* gkGetGlyphSet(gkGlyphCollection* collection, int index){
	gkGlyphSet* glyphSet;
	int setIndex = index>>collection->setBits;
	if(setIndex >= collection->glyphSetCount){
		int newSetCount = setIndex + 1;
		collection->glyphSets = (gkGlyphSet**)realloc(collection->glyphSets, newSetCount*sizeof(gkGlyphSet*));
		memset(collection->glyphSets + collection->glyphSetCount, 0, (newSetCount - collection->glyphSetCount)*sizeof(gkGlyphSet*));
		collection->glyphSetCount = newSetCount;
	}
	if(collection->glyphSets[setIndex] == 0){
		int setSize = (0xFF>>(8 - collection->setBits)) + 1;
		int w = collection->texWidth, h = collection->texHeight;
		char* tmp = 0;
//#ifdef GK_PLATFORM_WEB
		size_t tmpSize = (w*h)*4;
		tmp = (char*)malloc(tmpSize); // nasty, can't put 0 in WebGL's glTexImage2D
		memset(tmp, 0, tmpSize);
//#endif
		glyphSet = (gkGlyphSet*)malloc(sizeof(gkGlyphSet));
		glGenTextures(1, &glyphSet->texId);
		glBindTexture(GL_TEXTURE_2D, glyphSet->texId);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);

		if (tmp)
			free(tmp);

		glyphSet->glyphs = (gkGlyph**)calloc(setSize, sizeof(gkGlyph*));
		memset(glyphSet->glyphs, 0, setSize*sizeof(gkGlyph*));
		collection->glyphSets[setIndex] = glyphSet;
	}else{
		glyphSet = collection->glyphSets[setIndex];
	}
	return glyphSet;
}

void gkGetCellPos(gkGlyphCollection* collection, int index, int *x, int *y){
	int glyphIndex = index&(0xFF>>(8 - collection->setBits));
	int cols = (collection->texWidth/collection->cellWidth);
	int column = glyphIndex % cols;
	int row = glyphIndex / cols;
	*x = column*collection->cellWidth;
	*y = row*collection->cellHeight;
}

static void printGlyphError(FT_Error error)
{
	if (error == FT_Err_Invalid_Glyph_Index) {
		printf("GK [ERROR]: Invalid Glyph Index\n");
	} else if(error == FT_Err_Cannot_Render_Glyph) {
		printf("GK [ERROR]: Cannot Render Glyph\n");
	}
}

gkGlyph* gkMakeGlyph(FT_Face face, gkGlyphCollection* collection, gkGlyphSet* glyphSet, int index){
	gkGlyph* glyph;
	FT_GlyphSlot slot = face->glyph;
	FT_BBox cbox;
	FT_Glyph ftglyph;
	FT_Stroker stroker;
	FT_BitmapGlyph bglyph;
	FT_Error error;
	uint8_t *buf, *pbuf;
	int r, tr, tx, ty;
	float texWidth = (float)collection->texWidth, texHeight = (float)collection->texHeight;

	if(collection->strokeSize == 0){

		error = FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
		if (error) {
			printGlyphError(error);
			return 0;
		}

		error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);;
		if (error) {
			printGlyphError(error);
			return 0;
		}

		glyph = (gkGlyph*)malloc(sizeof(gkGlyph));

		pbuf = buf = (uint8_t*)malloc(slot->bitmap.width*slot->bitmap.rows*4*sizeof(uint8_t));
		for(r = 0; r<slot->bitmap.rows; r++){
			uint8_t *bmp = slot->bitmap.buffer + r*slot->bitmap.pitch;
			pbuf = buf + r*slot->bitmap.width*4;
			for(tr = 0; tr<slot->bitmap.width; tr++){
				*pbuf++ = 0xFF;
				*pbuf++ = 0xFF;
				*pbuf++ = 0xFF;
				*pbuf++ = *bmp++;
			}
		}

		glyph->size.width = (float)slot->bitmap.width;
		glyph->size.height = (float)slot->bitmap.rows;

		glyph->offset.x = (float)slot->bitmap_left;
		glyph->offset.y = (float)slot->bitmap_top;

		glyph->advance.x = ((float)slot->metrics.horiAdvance)/64.0f;
		glyph->advance.y = ((float)slot->metrics.vertAdvance)/64.0f;

		FT_Get_Glyph(slot, &ftglyph);
		FT_Glyph_Get_CBox(ftglyph, ft_glyph_bbox_pixels, &cbox);
		FT_Done_Glyph(ftglyph);

		glyph->bbox = gkCreateBBox((float)(cbox.xMin), (float)(-cbox.yMax), (float)(cbox.xMax), (float)(-cbox.yMin));

		glyph->texId = glyphSet->texId;

		gkGetCellPos(collection, index, &tx, &ty);

		glBindTexture(GL_TEXTURE_2D, glyphSet->texId);
		glTexSubImage2D(GL_TEXTURE_2D, 0, tx, ty, slot->bitmap.width, slot->bitmap.rows, GL_RGBA, GL_UNSIGNED_BYTE, buf);

		glyph->texCoords.x = (float)tx/texWidth;
		glyph->texCoords.y = (float)ty/texHeight;
		glyph->texCoords.width = (float)(tx + slot->bitmap.width)/texWidth - glyph->texCoords.x;
		glyph->texCoords.height = (float)(ty + slot->bitmap.rows)/texHeight - glyph->texCoords.y;
		free(buf);

	}else{
		if(FT_Load_Glyph(face, index, FT_LOAD_DEFAULT)) return 0;

		FT_Stroker_New(ftlib, &stroker);
		FT_Stroker_Set(stroker, (FT_Fixed)(collection->strokeSize*16.0f), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

		FT_Get_Glyph(slot, &ftglyph);
		if(FT_Glyph_Stroke(&ftglyph, stroker, GK_TRUE) != 0){
			FT_Done_Glyph(ftglyph);
			return 0;
		}

		FT_Glyph_To_Bitmap(&ftglyph, FT_RENDER_MODE_NORMAL, 0, 1);
		bglyph = (FT_BitmapGlyph)ftglyph;

		glyph = (gkGlyph*)malloc(sizeof(gkGlyph));

		pbuf = buf = (uint8_t*)malloc(bglyph->bitmap.width*bglyph->bitmap.rows*4*sizeof(uint8_t));
		for(r = 0; r<bglyph->bitmap.rows; r++){
			uint8_t *bmp = bglyph->bitmap.buffer + r*bglyph->bitmap.pitch;
			pbuf = buf + r*bglyph->bitmap.width*4;
			for(tr = 0; tr<bglyph->bitmap.width; tr++){
				*pbuf++ = 0xFF;
				*pbuf++ = 0xFF;
				*pbuf++ = 0xFF;
				*pbuf++ = *bmp++;
			}
		}

		FT_Stroker_Done(stroker);

		glyph->size.width = (float)bglyph->bitmap.width;
		glyph->size.height = (float)bglyph->bitmap.rows;

		glyph->offset.x = (float)bglyph->left;
		glyph->offset.y = (float)bglyph->top;

		glyph->advance.x = ((float)slot->metrics.horiAdvance)/64.0f;
		glyph->advance.y = ((float)slot->metrics.vertAdvance)/64.0f;

		glyph->texId = glyphSet->texId;

		gkGetCellPos(collection, index, &tx, &ty);

		glBindTexture(GL_TEXTURE_2D, glyphSet->texId);
		glTexSubImage2D(GL_TEXTURE_2D, 0, tx, ty, bglyph->bitmap.width, bglyph->bitmap.rows, GL_RGBA, GL_UNSIGNED_BYTE, buf);

		glyph->texCoords.x = (float)tx/texWidth;
		glyph->texCoords.y = (float)ty/texHeight;
		glyph->texCoords.width = (float)(tx + bglyph->bitmap.width)/texWidth - glyph->texCoords.x;
		glyph->texCoords.height = (float)(ty + bglyph->bitmap.rows)/texHeight - glyph->texCoords.y;

		free(buf);
	}
	glyph->index = index;
	return glyph;
}

void gkDestroyFace(gkFontFaceEx* face){
	gkGlyphCollection* p = face->collections, *r;
	while(p){
		p = (r = p)->next;
		gkDestroyGlyphCollection(r);
	}
	FT_Done_Face(face->ftface);
	free(face);
}

void gkDestroyGlyphCollection(gkGlyphCollection* collection){
	int i;
	int setSize = 0xFF>>(8 - collection->setBits);
	for(i = 0; i<collection->glyphSetCount; i++){
		gkGlyphSet* set = collection->glyphSets[i];
		if(set) gkDestroyGlyphSet(set, setSize);
	}
	free(collection->glyphSets);
	free(collection);
}

void gkDestroyGlyphSet(gkGlyphSet* set, int setSize){
	int i;
	for(i = 0; i<setSize; i++){
		if(set->glyphs[i]) free(set->glyphs[i]);
	}
	free(set->glyphs);
	glDeleteTextures(1, &set->texId);
	free(set);
}

/*
	Drawing and measuring
*/



/*
	Batch-rendering for fonts
*/

#define QUAD_SIZE sizeof(float)*12 /* 6 vertices x 2 floats */

static float *batchVertices = 0;
static float *batchCoords = 0;

static int quadIndex = 0;
static int totalQuads = 0;

static void batchEnlarge()
{
	if (totalQuads == 0) {
		totalQuads = 128;
	}

	totalQuads = totalQuads << 1;
	batchVertices = (float*)realloc(batchVertices, QUAD_SIZE*totalQuads);
	batchCoords = (float*)realloc(batchCoords, QUAD_SIZE*totalQuads);
}

static void batchPush(gkPoint p, gkGlyph *g)
{
	float v[] = {
		p.x, p.y,
		p.x + g->size.width, p.y,
		p.x + g->size.width, p.y + g->size.height,
		p.x, p.y + g->size.height
	};
	float c[] = {
		g->texCoords.x, g->texCoords.y,
		g->texCoords.x + g->texCoords.width, g->texCoords.y,
		g->texCoords.x + g->texCoords.width, g->texCoords.y + g->texCoords.height,
		g->texCoords.x, g->texCoords.y + g->texCoords.height,
	};
	float *bv;
	float *bc;

	if (quadIndex + 1 >= totalQuads)
		batchEnlarge();

	/* GL_TRIANGLES */
/*	bv = batchVertices + quadIndex*12;
	bc = batchCoords + quadIndex*12;
	bv[0] = v[0]; //v 1
	bv[1] = v[1]; //v 1
	bv[2] = v[2]; //v 2
	bv[3] = v[3]; //v 2
	bv[4] = v[4]; //v 3
	bv[5] = v[5]; //v 3
	bv[6] = v[0]; //v 1
	bv[7] = v[1]; //v 1
	bv[8] = v[4]; //v 3
	bv[9] = v[5]; //v 3
	bv[10] = v[6]; //v 4
	bv[11] = v[7]; //v 4

	bc[0] = c[0]; //v 1
	bc[1] = c[1]; //v 1
	bc[2] = c[2]; //v 2
	bc[3] = c[3]; //v 2
	bc[4] = c[4]; //v 3
	bc[5] = c[5]; //v 3
	bc[6] = c[0]; //v 1
	bc[7] = c[1]; //v 1
	bc[8] = c[4]; //v 3
	bc[9] = c[5]; //v 3
	bc[10] = c[6]; //v 4
	bc[11] = c[7]; //v 4
	*/


	/* GL_TRIANGLE_STRIPS */
	if (quadIndex == 0) {
		bv = batchVertices;
		bc = batchCoords;
	} else {
		bv = batchVertices + quadIndex*12;
		bc = batchCoords + quadIndex*12;
	}//2 - 3 - 1 - 4 
	bv[0] = v[2]; //v 2
	bv[1] = v[3]; //v 2
	bv[2] = v[4]; //v 3
	bv[3] = v[5]; //v 3
	bv[4] = v[0]; //v 1
	bv[5] = v[1]; //v 1
	bv[6] = v[6]; //v 4
	bv[7] = v[7]; //v 4

	bc[0] = c[2]; //v 2
	bc[1] = c[3]; //v 2
	bc[2] = c[4]; //v 3
	bc[3] = c[5]; //v 3
	bc[4] = c[0]; //v 1
	bc[5] = c[1]; //v 1
	bc[6] = c[6]; //v 4
	bc[7] = c[7]; //v 4

	if (quadIndex != 0) {
		bv[-4] = bv[-6];
		bv[-3] = bv[-5];
		bv[-2] = bv[0];
		bv[-1] = bv[1];

		bc[-4] = bc[-6];
		bc[-3] = bc[-5];
		bc[-2] = bc[0];
		bc[-1] = bc[1];
	}

	quadIndex++;
}

static void batchDraw()
{
	if (quadIndex == 0)
		return;

	glVertexPointer(2, GL_FLOAT, 0, batchVertices);
	glTexCoordPointer(2, GL_FLOAT, 0, batchCoords);
//	glDrawArrays(GL_TRIANGLES, 0, quadIndex*6);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4 + (quadIndex-1)*6);
	quadIndex = 0;
}

static void cleanupBatch()
{
	if (totalQuads>0) {
		free(batchVertices);
		free(batchCoords);
	}
}

/*********/

static float gkRound(float r)
{
	float s = (r<0.0f?-1.0f:1.0f);
	int a = (int)(r + s*0.5f);
	return (float)a;
}

/**/

typedef struct gkSentenceElementStruct gkSentenceElement;
struct gkSentenceElementStruct{
	uint8_t type;
	gkPoint advance;
	gkBBox bbox;
	gkGlyph** glyphStart;
	gkGlyph** glyphEnd;
	gkGlyph** strokeStart;
	gkGlyph** strokeEnd;
	gkSentenceElement* next;
};
typedef struct gkSentenceLineStruct gkSentenceLine;
struct gkSentenceLineStruct{
	gkSentenceElement* first;
	gkSentenceElement* last;
	gkBBox bbox;
	gkSentenceLine* next;
};

#define GK_SE_WORD		0
#define GK_SE_TAB		1
#define GK_SE_SPACE		2
#define GK_SE_NEWLINE	3

gkSentenceElement* gkParseSentenceElements(gkFont* font, char* text, gkTextFormat* format);
void gkFreeSentenceElements(gkSentenceElement* elements);
gkSentenceLine* gkParseSentenceLines(gkFont* font, gkSentenceElement* elements, gkTextFormat* format);
void gkFreeSentenceLines(gkSentenceLine* lines);
gkBBox gkGetTextBBox(gkSentenceLine* lines, gkTextFormat* format, float leading);
gkPoint gkAlignLine(gkSentenceLine* line, gkTextFormat* format, gkBBox* textBBox);
gkPoint gkDrawSentenceLine(FT_Face face, gkSentenceLine* line, gkTextFormat* format);

gkSize gkMeasureText(gkFont* font, char* text, gkTextFormat* format)
{
	FT_Face face = ((gkFontFaceEx*)font->face)->ftface;
	gkSentenceElement* elements;
	gkSentenceLine* lines;
	float oldTextFormatWidth;
	gkBBox textBBox;

	if(format == 0)
        format = &gkDefaultTextFormat;

	oldTextFormatWidth = format->width;

	if(elements = gkParseSentenceElements(font, text, format))
    {
		float leading = ((float)face->size->metrics.height)/64.0f + format->lineSpacing;
		lines = gkParseSentenceLines(font, elements, format);
		textBBox = gkGetTextBBox(lines, format, leading);
		gkFreeSentenceLines(lines);
		gkFreeSentenceElements(elements);
	}else{
	    textBBox.minX = textBBox.maxX = 0;
	    textBBox.minY = textBBox.maxY = 0;
	}
	format->width = oldTextFormatWidth;
	return GK_SIZE(textBBox.maxX - textBBox.minX, textBBox.maxY - textBBox.minY);
}

void gkDrawText(gkFont* font, char* text, float x, float y, gkTextFormat* format){
	FT_Face face = ((gkFontFaceEx*)font->face)->ftface;
	gkSentenceElement* elements;
	gkSentenceLine* lines, *currentLine;
	float oldTextFormatWidth;
	if(format == 0) format = &gkDefaultTextFormat;
	oldTextFormatWidth = format->width;

	if(elements = gkParseSentenceElements(font, text, format)){
		gkPoint align;
		float tx = x, ty = y;
		float leading = gkRound(((float)face->size->metrics.height)/64.0f + format->lineSpacing);
		gkBBox textBBox;
		currentLine = lines = gkParseSentenceLines(font, elements, format);
		textBBox = gkGetTextBBox(lines, format, leading);
		while(currentLine){
			glPushMatrix();
			align = gkAlignLine(currentLine, format, &textBBox);
			glTranslatef(tx + align.x,ty + align.y,0);
			gkDrawSentenceLine(face, currentLine, format);
			glPopMatrix();
			currentLine = currentLine->next;
			if(format->vertical){
				tx -= leading;
			}else{
				ty += leading;
			}
		}
		gkFreeSentenceLines(lines);
		gkFreeSentenceElements(elements);
	}
	format->width = oldTextFormatWidth;
}

gkSentenceElement* gkParseSentenceElements(gkFont* font, char* text, gkTextFormat* format){
	FT_Face face = ((gkFontFaceEx*)font->face)->ftface;
	GK_BOOL hasKerning = FT_HAS_KERNING(face);

	size_t totalGlyphs = 0, currentGlyph = 0;
	gkGlyph** glyphs, **strokes = 0;
	gkGlyphCollection* collection, *strokeCollection;
	gkSentenceElement* firstElement = 0, *lastElement = firstElement, *currentElement = 0;

	char* str;
	uint32_t c, lastChar;

	str = gkUtf8CharCode(text, &c);
	while (c) {
		if(c != CHAR(' ') && c != CHAR('\t') && c != CHAR('\r') && c != CHAR('\n')) 
			totalGlyphs++;
		str = gkUtf8CharCode(str, &c);
	}

	{
		int index, prevIndex = 0;
		gkPoint spaceAdvance;
		gkGlyph* glyph = 0;

		FT_Set_Char_Size(face, 0, font->size*64, 0, 96);
		collection = gkGetGlyphCollection(font, 0);

		if (totalGlyphs > 0) {
			glyphs = (gkGlyph**)calloc(totalGlyphs, sizeof(gkGlyph*));
			if(format->strokeSize>0.0f){
				strokes = (gkGlyph**)calloc(totalGlyphs, sizeof(gkGlyph*));
				strokeCollection = gkGetGlyphCollection(font, format->strokeSize);
			}
		}

		spaceAdvance.x = collection->spaceAdvance.x;
		spaceAdvance.y = collection->spaceAdvance.y;

		str = gkUtf8CharCode(text, &c);
		lastChar = c;
		while(c) {
			if(c != CHAR(' ') && c != CHAR('\t') 
					&& c != CHAR('\r') && c != CHAR('\n')) {
				index = FT_Get_Char_Index(face, c);
				if(strokes) strokes[currentGlyph] = gkGetGlyph(face, strokeCollection, gkGetGlyphSet(strokeCollection, index), index);
				glyphs[currentGlyph] = glyph = gkGetGlyph(face, collection, gkGetGlyphSet(collection, index), index);
				if(currentElement && currentElement->type != GK_SE_WORD){
					lastElement->next = currentElement;
					lastElement = currentElement;
					currentElement = 0;
				}
				if(currentElement == 0){
					currentElement = (gkSentenceElement*)malloc(sizeof(gkSentenceElement));
					currentElement->type = GK_SE_WORD;
					currentElement->glyphStart = glyphs + currentGlyph;
					currentElement->glyphEnd = glyphs + currentGlyph;
					currentElement->strokeStart = strokes + currentGlyph;
					currentElement->strokeEnd = strokes + currentGlyph;
					currentElement->advance.x = glyph->advance.x;
					currentElement->advance.y = glyph->advance.y;
					currentElement->bbox = glyph->bbox;
					currentElement->next = 0;
				}else{
					gkBBox tbbox = glyph->bbox;
					currentElement->glyphEnd = glyphs + currentGlyph;
					currentElement->strokeEnd = strokes + currentGlyph;
					if(format->vertical){
						gkBBoxTranslate(&tbbox, 0, currentElement->advance.y);
					}else{
						gkBBoxTranslate(&tbbox, currentElement->advance.x, 0);
					}
					gkBBoxAdd(&currentElement->bbox, &tbbox);
					currentElement->advance.x += glyph->advance.x;
					currentElement->advance.y += glyph->advance.y;
					if(hasKerning){
						FT_Vector delta;
						FT_Get_Kerning(face, prevIndex, index, FT_KERNING_DEFAULT, &delta);
						currentElement->advance.x += ((float)delta.x)/64.0f;
						currentElement->advance.y += ((float)delta.y)/64.0f;
					}
				}
				prevIndex = index;
				currentGlyph++;
			}else{
				if(currentElement && currentElement->type == GK_SE_WORD){
					lastElement->next = currentElement;
					lastElement = currentElement;
				}
				currentElement = 0;
				if(c == CHAR('\t')){
					currentElement = (gkSentenceElement*)malloc(sizeof(gkSentenceElement));
					currentElement->type = GK_SE_TAB;
					currentElement->advance = spaceAdvance;
					if(format->vertical){
						currentElement->bbox = gkCreateBBox(0, 0, 0, spaceAdvance.y);
					}else{
						currentElement->bbox = gkCreateBBox(0, 0, spaceAdvance.x, 0);
					}
					currentElement->next = 0;
				}else if(c == CHAR(' ')) {
					currentElement = (gkSentenceElement*)malloc(sizeof(gkSentenceElement));
					currentElement->type = GK_SE_SPACE;
					currentElement->advance = spaceAdvance;
					if(format->vertical){
						currentElement->bbox = gkCreateBBox(0, 0, 0, spaceAdvance.y);
					}else{
						currentElement->bbox = gkCreateBBox(0, 0, spaceAdvance.x, 0);
					}
					currentElement->next = 0;
				}else if((c == CHAR('\n') && lastChar != CHAR('\r')) || c == CHAR('\r')) {
					currentElement = (gkSentenceElement*)malloc(sizeof(gkSentenceElement));
					currentElement->type = GK_SE_NEWLINE;
					currentElement->bbox = gkCreateBBox(0,0,0,0);
					currentElement->next = 0;
				}
				if(currentElement && lastElement){
					lastElement->next = currentElement;
					lastElement = currentElement;
					currentElement = 0;
				}
				prevIndex  = 0;
			}
			if(firstElement == 0 && currentElement){
				firstElement = lastElement = currentElement;
			}
			lastChar = c;
			str = gkUtf8CharCode(str, &c);
		}
		if(currentElement && currentElement != lastElement){
			lastElement->next = currentElement;
			lastElement = currentElement;
			currentElement = 0;
		}
	}
	return firstElement;
}

void gkFreeSentenceElements(gkSentenceElement* elements){
	gkSentenceElement* p = elements, *r;
	GK_BOOL glyphsFreed = GK_FALSE;
	while(p){
		if (!glyphsFreed && p->type == GK_SE_WORD)
		{
			free(p->strokeStart);
			free(p->glyphStart);
			glyphsFreed = GK_TRUE;
		}
		p = (r = p)->next;
		free(r);
	}
}

gkSentenceLine* gkParseSentenceLines(gkFont* font, gkSentenceElement* elements, gkTextFormat* format){
	FT_Face face = ((gkFontFaceEx*)font->face)->ftface;
	gkSentenceElement* currentElement = elements;
	gkSentenceLine *firstLine = (gkSentenceLine*)malloc(sizeof(gkSentenceLine)), *current = firstLine, *newLine;
	gkPoint advance = {0,0};
	GK_BOOL keepBaseline = format->keepBaseline;
	GK_BOOL wordWrap = format->wordWrap && format->width>0;
	float ascender = -FT_MulFix(face->ascender, face->size->metrics.y_scale)/64;
	float descender = -FT_MulFix(face->descender, face->size->metrics.y_scale)/64;

	memset(firstLine, 0, sizeof(gkSentenceLine));
	while(currentElement){
		if(currentElement->type == GK_SE_NEWLINE){
wordWrap:
			if (keepBaseline) {
				current->bbox.minY = ascender;
				current->bbox.maxY = descender;
			}
			newLine = (gkSentenceLine*)malloc(sizeof(gkSentenceLine));
			memset(newLine, 0, sizeof(gkSentenceLine));
			current->next = newLine;
			current = newLine;
		}else{
			if(current->first == 0){
				current->first = current->last = currentElement;
				current->bbox = currentElement->bbox;
				advance = currentElement->advance;
			}else{
				gkBBox tbbox = currentElement->bbox;
				if(format->vertical){
					gkBBoxTranslate(&tbbox, 0, advance.y);
				}else{
					gkBBoxTranslate(&tbbox, advance.x, 0);
				}
				gkBBoxAdd(&current->bbox, &tbbox);
				current->last = currentElement;
				advance.x += currentElement->advance.x;
				advance.y += currentElement->advance.y;
				if(wordWrap && currentElement->next)
				{
					gkBBox* nextBox = &currentElement->next->bbox;
					float nextW = (nextBox->maxX - nextBox->minX) + (current->bbox.maxX - current->bbox.minX);
					if(nextW > format->width)
					{
						if(currentElement->next->type != GK_SE_WORD)
							currentElement = currentElement->next;
						goto wordWrap;
					}
				}
			}
		}
		currentElement = currentElement->next;
	}
	if (keepBaseline) {
		current->bbox.minY = ascender;
		current->bbox.maxY = descender;
	}
	return firstLine;
}

void gkFreeSentenceLines(gkSentenceLine* lines){
	gkSentenceLine* p;
	while(lines){
		lines = (p = lines)->next;
		free(p);
	}
}

gkBBox gkGetTextBBox(gkSentenceLine* lines, gkTextFormat* format, float leading){
	gkBBox res = lines->bbox;
	gkSentenceLine* currentLine = lines;
	float tx = 0, ty = 0;
	while(currentLine){
		gkBBox b = currentLine->bbox;
		gkBBoxTranslate(&b, tx, ty);
		gkBBoxAdd(&res, &b);
		if(format->vertical){
			tx -= leading;
		}else{
			ty += leading;
		}
		currentLine = currentLine->next;
	}
	return res;
}
gkPoint gkAlignLine(gkSentenceLine* line, gkTextFormat* format, gkBBox* textBBox){
	gkPoint result = {0,0};
	float width;
	float height;
	if(format->width == 0){
		width = textBBox->maxX - textBBox->minX;
	}else{
		width = format->width;
	}
	if(format->height == 0){
		height = textBBox->maxY - textBBox->minY;
	}else{
		height = format->height;
	}
	if(format->vertical){
		if(format->align == GK_TEXT_ALIGN_LEFT){
			result.x = -textBBox->minX;
		}else if(format->align == GK_TEXT_ALIGN_CENTER){
			result.x = (width - (textBBox->maxX - textBBox->minX))/2 - textBBox->minX;
		}else if(format->align == GK_TEXT_ALIGN_RIGHT){
			result.x = (width - (textBBox->maxX - textBBox->minX)) - textBBox->minX;
		}
		if(format->valign == GK_TEXT_VALIGN_TOP){
			result.y = -line->bbox.minY;
		}else if(format->valign == GK_TEXT_VALIGN_MIDDLE){
			result.y = (height - (line->bbox.maxY - line->bbox.minY))/2 - line->bbox.minY;
		}else if(format->valign == GK_TEXT_VALIGN_BOTTOM){
			result.y = (height - (line->bbox.maxY - line->bbox.minY)) - line->bbox.minY;
		}
	}else{
		if(format->align == GK_TEXT_ALIGN_LEFT){
			result.x = -line->bbox.minX;
		}else if(format->align == GK_TEXT_ALIGN_CENTER){
			result.x = (width - (line->bbox.maxX - line->bbox.minX))/2 - line->bbox.minX;
		}else if(format->align == GK_TEXT_ALIGN_RIGHT){
			result.x = (width - (line->bbox.maxX - line->bbox.minX)) - line->bbox.minX;
		}
		if(format->valign == GK_TEXT_VALIGN_TOP){
			result.y = -textBBox->minY;
		}else if(format->valign == GK_TEXT_VALIGN_MIDDLE){
			result.y = (height - (textBBox->maxY - textBBox->minY))/2 - textBBox->minY;
		}else if(format->valign == GK_TEXT_VALIGN_BOTTOM){
			result.y = (height - (textBBox->maxY - textBBox->minY)) - textBBox->minY;
		}
	}
	result.x = gkRound(result.x);
	result.y = gkRound(result.y);
	return result;
}

gkPoint gkDrawSentenceLine(FT_Face face, gkSentenceLine* line, gkTextFormat* format){
	gkGlyph *g, **c, **firstGlyph, **lastGlyph;
	gkPoint b = GK_POINT(0,0), p;
	GLuint texImage = 0;
	int prevIndex = 0;
	GK_BOOL stroke = format->strokeSize>0;
	GK_BOOL hasKerning = FT_HAS_KERNING(face);
	gkSentenceElement* currentElement;
	gkColor tmpColor;
	if(line->first == 0) return GK_POINT(0,0);
draw:
	if(stroke){
		tmpColor = gkGetFilteredColor(format->strokeColor);
	}else{
		tmpColor = gkGetFilteredColor(format->textColor);
	}
	if(format->underline && format->vertical == GK_FALSE){
		float underlinePos = ((float)FT_MulFix(face->underline_position, face->size->metrics.y_scale))/64.0f;
		float underlineThickness = ((float)FT_MulFix(face->underline_thickness, face->size->metrics.y_scale))/64.0f;
		if(stroke){
			float lw = format->strokeSize/3.0f;
			underlineThickness += lw;
			gkSetLineWidth(lw);
			gkSetLineColor(tmpColor.r, tmpColor.g, tmpColor.b, tmpColor.a);
			gkSetFillColor(0,0,0,0);
		}else{
			gkSetLineWidth(0);
			gkSetFillColor(tmpColor.r, tmpColor.g, tmpColor.b, tmpColor.a);
		}
		gkDrawRect(-underlineThickness*0.5f + line->bbox.minX, -underlinePos - underlineThickness*0.5f, underlineThickness + (line->bbox.maxX - line->bbox.minX), underlineThickness);
	}
	glColor4f(tmpColor.r, tmpColor.g, tmpColor.b, tmpColor.a);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	currentElement = line->first;
	while(1){
		if(currentElement->type == GK_SE_WORD){
			if(stroke){
				firstGlyph = currentElement->strokeStart;
				lastGlyph = currentElement->strokeEnd;
			}else{
				firstGlyph = currentElement->glyphStart;
				lastGlyph = currentElement->glyphEnd;
			}
			for(c = firstGlyph; c <= lastGlyph; c++){
				g = *c;
				if(hasKerning){
					FT_Vector delta;
					FT_Get_Kerning(face, prevIndex, g->index, FT_KERNING_DEFAULT, &delta);
					if(format->vertical){
						b.y += ((float)delta.y)/64.0f;
					}else{
						b.x += ((float)delta.x)/64.0f;
					}
				}
				p.x = b.x + g->offset.x;
				p.y = b.y - g->offset.y;
				if(texImage == 0 || texImage != g->texId){
					batchDraw();
					texImage = g->texId;
					glBindTexture(GL_TEXTURE_2D, texImage);
				}
/*				{
					float v[] = {
						p.x, p.y,
						p.x + g->size.width, p.y,
						p.x + g->size.width, p.y + g->size.height,
						p.x, p.y + g->size.height
					};
					float c[] = {
						g->texCoords.x, g->texCoords.y,
						g->texCoords.x + g->texCoords.width, g->texCoords.y,
						g->texCoords.x + g->texCoords.width, g->texCoords.y + g->texCoords.height,
						g->texCoords.x, g->texCoords.y + g->texCoords.height,
					};
					glVertexPointer(2, GL_FLOAT, 0, v);
					glTexCoordPointer(2, GL_FLOAT, 0, c);
					glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
				}*/
				batchPush(p, g);

				if(format->vertical){
					b.y += g->advance.y;
				}else{
					b.x += g->advance.x;
				}
				prevIndex = g->index;
			}
		}else if(currentElement->type == GK_SE_SPACE){
			if(format->vertical){
				b.y += currentElement->advance.y;
			}else{
				b.x += currentElement->advance.x;
			}
		}else if(currentElement->type == GK_SE_TAB){
			//make tabulations
		}
		if(currentElement == line->last) break;
		currentElement = currentElement->next;
	}
	batchDraw();
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);

	if(stroke){
		b = GK_POINT(0,0);
		prevIndex = 0;
		stroke = GK_FALSE;
		goto draw;
	}
	return GK_POINT(0,0);
}

#endif	//GK_USE_FONTS