/** 
 * @file llfont.cpp
 * @brief Font library wrapper
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llfont.h"

// Freetype stuff
#if !defined(LL_LINUX) || defined(LL_STANDALONE)
# include <ft2build.h>
#else
// I had to do some work to avoid the system-installed FreeType headers... --ryan.
//This path no longer exists.
//# include "llfreetype2/freetype/ft2build.h"
//This works fine.
# include "../include/ft2build.h"
#endif

// For some reason, this won't work if it's not wrapped in the ifdef
#ifdef FT_FREETYPE_H
#include FT_FREETYPE_H
#endif

#include "llerror.h"
#include "llimage.h"
//#include "llimagej2c.h"
#include "llmath.h"	// Linden math
#include "llstring.h"
//#include "imdebug.h"
#include "llfontbitmapcache.h"
#include "llgl.h"

FT_Render_Mode gFontRenderMode = FT_RENDER_MODE_NORMAL;

LLFontManager *gFontManagerp = NULL;

FT_Library gFTLibrary = NULL;

bool LLFont::sOpenGLcrashOnRestart = false;

//static
void LLFontManager::initClass()
{
	gFontManagerp = new LLFontManager;
}

//static
void LLFontManager::cleanupClass()
{
	delete gFontManagerp;
	gFontManagerp = NULL;
}

LLFontManager::LLFontManager()
{
	int error;
	error = FT_Init_FreeType(&gFTLibrary);
	if (error)
	{
		// Clean up freetype libs.
		llerrs << "Freetype initialization failure!" << llendl;
		FT_Done_FreeType(gFTLibrary);
	}
}


LLFontManager::~LLFontManager()
{
	FT_Done_FreeType(gFTLibrary);
}


LLFontGlyphInfo::LLFontGlyphInfo(U32 index)
:	mGlyphIndex(index),
	mXBitmapOffset(0), 	// Offset to the origin in the bitmap
	mYBitmapOffset(0), 	// Offset to the origin in the bitmap
	mXBearing(0),		// Distance from baseline to left in pixels
	mYBearing(0),		// Distance from baseline to top in pixels
	mWidth(0),			// In pixels
	mHeight(0),			// In pixels
	mXAdvance(0.f),		// In pixels
	mYAdvance(0.f),		// In pixels
	mIsRendered(FALSE),
	mMetricsValid(FALSE)
{}

LLFontList::LLFontList()
{
}

LLFontList::~LLFontList()
{
	LLFontList::iterator iter;
	for(iter = this->begin(); iter != this->end(); iter++)
	{
		delete *iter;
		// The (now dangling) pointers in the vector will be cleaned up when the vector is deleted by the superclass destructor.
	}
}
void LLFontList::addAtEnd(LLFont *font)
{
	// Purely a convenience function
	this->push_back(font);
}

LLFont::LLFont()
{
	mFontBitmapCachep = new LLFontBitmapCache;

	mValid = FALSE;
	mAscender = 0.f;
	mDescender = 0.f;
	mLineHeight = 0.f;

	mFallbackFontp = NULL;
	mIsFallback = FALSE;
	mFTFace = NULL;

	mRenderGlyphCount = 0;
	mAddGlyphCount = 0;

	mPointSize = 0;
}


LLFont::~LLFont()
{
	// Clean up freetype libs.
	if (mFTFace)
		FT_Done_Face(mFTFace);
	mFTFace = NULL;

	// Delete glyph info
	std::for_each(mCharGlyphInfoMap.begin(), mCharGlyphInfoMap.end(), DeletePairedPointer());

	// mFontBitmapCachep will be cleaned up by LLPointer destructor.
}

// virtual
F32 LLFont::getLineHeight() const
{
	return mLineHeight;
}

// virtual
F32 LLFont::getAscenderHeight() const
{
	return mAscender;
}

// virtual
F32 LLFont::getDescenderHeight() const
{
	return mDescender;
}

BOOL LLFont::loadFace(const std::string& filename, const F32 point_size, const F32 vert_dpi, const F32 horz_dpi, const S32 components, BOOL is_fallback)
{
	// Don't leak face objects.  This is also needed to deal with
	// changed font file names.
	if (mFTFace)
	{
		FT_Done_Face(mFTFace);
		mFTFace = NULL;
	}

	int error;

	error = FT_New_Face( gFTLibrary,
						 filename.c_str(),
						 0,
						 &mFTFace );

    if (error)
	{
		return FALSE;
	}

	mIsFallback = is_fallback;
	F32 pixels_per_em = (point_size / 72.f)*vert_dpi; // Size in inches * dpi

	error = FT_Set_Char_Size(mFTFace,    /* handle to face object           */
							0,       /* char_width in 1/64th of points  */
							(S32)(point_size*64),   /* char_height in 1/64th of points */
							(U32)horz_dpi,     /* horizontal device resolution    */
							(U32)vert_dpi);   /* vertical device resolution      */

	if (error)
	{
		// Clean up freetype libs.
		FT_Done_Face(mFTFace);
		mFTFace = NULL;
		return FALSE;
	}

	F32 y_max, y_min, x_max, x_min;
	F32 ems_per_unit = 1.f/ mFTFace->units_per_EM;
	F32 pixels_per_unit = pixels_per_em * ems_per_unit;

	// Get size of bbox in pixels
	y_max = mFTFace->bbox.yMax * pixels_per_unit;
	y_min = mFTFace->bbox.yMin * pixels_per_unit;
	x_max = mFTFace->bbox.xMax * pixels_per_unit;
	x_min = mFTFace->bbox.xMin * pixels_per_unit;
	mAscender = mFTFace->ascender * pixels_per_unit;
	mDescender = -mFTFace->descender * pixels_per_unit;
	mLineHeight = mFTFace->height * pixels_per_unit;

	S32 max_char_width = llround(0.5f + (x_max - x_min));
	S32 max_char_height = llround(0.5f + (y_max - y_min));

	mFontBitmapCachep->init(components, max_char_width, max_char_height);

	if (!mFTFace->charmap)
	{
		//llinfos << " no unicode encoding, set whatever encoding there is..." << llendl;
		FT_Set_Charmap(mFTFace, mFTFace->charmaps[0]);
	}

	if (!mIsFallback || !sOpenGLcrashOnRestart)	// because this often crashes under Linux...
	{
		// Add the empty glyph`5
		addGlyph(0, 0);
	}

	mName = filename;
	mPointSize = point_size;

	return TRUE;
}

void LLFont::resetBitmapCache()
{
	// Iterate through glyphs and clear the mIsRendered flag
	for (char_glyph_info_map_t::iterator iter = mCharGlyphInfoMap.begin();
		 iter != mCharGlyphInfoMap.end(); ++iter)
	{
		iter->second->mIsRendered = FALSE;
		//FIXME: this is only strictly necessary when resetting the entire font, 
		//not just flushing the bitmap
		iter->second->mMetricsValid = FALSE;
	}
	mFontBitmapCachep->reset();

	if (!mIsFallback || !sOpenGLcrashOnRestart)	// because this often crashes under Linux...
	{
	// Add the empty glyph`5
	addGlyph(0, 0);
	}
}

LLFontGlyphInfo* LLFont::getGlyphInfo(const llwchar wch) const
{
	char_glyph_info_map_t::iterator iter = mCharGlyphInfoMap.find(wch);
	if (iter != mCharGlyphInfoMap.end())
	{
		return iter->second;
	}
	return NULL;
}


BOOL LLFont::hasGlyph(const llwchar wch) const
{
	llassert(!mIsFallback);
	const LLFontGlyphInfo* gi = getGlyphInfo(wch);
	if (gi && gi->mIsRendered)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL LLFont::addChar(const llwchar wch) const
{
	if (mFTFace == NULL)
		return FALSE;

	llassert(!mIsFallback);
	//lldebugs << "Adding new glyph for " << wch << " to font" << llendl;

	FT_UInt glyph_index;

	// Initialize char to glyph map
	glyph_index = FT_Get_Char_Index(mFTFace, wch);
	if (glyph_index == 0)
	{
		// Try looking it up in the backup Unicode font
		if (mFallbackFontp)
		{
			//llinfos << "Trying to add glyph from fallback font!" << llendl;
			LLFontList::iterator iter;
			for(iter = mFallbackFontp->begin(); iter != mFallbackFontp->end(); iter++)
			{
				glyph_index = FT_Get_Char_Index((*iter)->mFTFace, wch);
				if (glyph_index)
				{
					addGlyphFromFont(*iter, wch, glyph_index);
					return TRUE;
				}
			}
		}
	}
	
	char_glyph_info_map_t::iterator iter = mCharGlyphInfoMap.find(wch);
	if (iter == mCharGlyphInfoMap.end() || !(iter->second->mIsRendered))
	{
		BOOL result = addGlyph(wch, glyph_index);
		return result;
	}
	return FALSE;
}

void LLFont::insertGlyphInfo(llwchar wch, LLFontGlyphInfo* gi) const
{
	char_glyph_info_map_t::iterator iter = mCharGlyphInfoMap.find(wch);
	if (iter != mCharGlyphInfoMap.end())
	{
		delete iter->second;
		iter->second = gi;
	}
	else
	{
		mCharGlyphInfoMap[wch] = gi;
	}
}

BOOL LLFont::addGlyphFromFont(const LLFont *fontp, const llwchar wch, const U32 glyph_index) const
{
	if (mFTFace == NULL)
		return FALSE;

	//llassert(!mIsFallback);
	fontp->renderGlyph(glyph_index);
	S32 width = fontp->mFTFace->glyph->bitmap.width;
	S32 height = fontp->mFTFace->glyph->bitmap.rows;

	S32 pos_x, pos_y;
	S32 bitmap_num;
	mFontBitmapCachep->nextOpenPos(width, pos_x, pos_y, bitmap_num);
	mAddGlyphCount++;

	LLFontGlyphInfo* gi = new LLFontGlyphInfo(glyph_index);
	gi->mXBitmapOffset = pos_x;
	gi->mYBitmapOffset = pos_y;
	gi->mBitmapNum = bitmap_num;
	gi->mWidth = width;
	gi->mHeight = height;
	gi->mXBearing = fontp->mFTFace->glyph->bitmap_left;
	gi->mYBearing = fontp->mFTFace->glyph->bitmap_top;
	// Convert these from 26.6 units to float pixels.
	gi->mXAdvance = fontp->mFTFace->glyph->advance.x / 64.f;
	gi->mYAdvance = fontp->mFTFace->glyph->advance.y / 64.f;
	gi->mIsRendered = TRUE;
	gi->mMetricsValid = TRUE;

	insertGlyphInfo(wch, gi);

	llassert(fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO
	    || fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

	if (fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO
	    || fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
	{
		U8 *buffer_data = fontp->mFTFace->glyph->bitmap.buffer;
		S32 buffer_row_stride = fontp->mFTFace->glyph->bitmap.pitch;
		U8 *tmp_graydata = NULL;

		if (fontp->mFTFace->glyph->bitmap.pixel_mode
		    == FT_PIXEL_MODE_MONO)
		{
			// need to expand 1-bit bitmap to 8-bit graymap.
			tmp_graydata = new U8[width * height];
			S32 xpos, ypos;
			for (ypos = 0; ypos < height; ++ypos)
			{
				S32 bm_row_offset = buffer_row_stride * ypos;
				for (xpos = 0; xpos < width; ++xpos)
				{
					U32 bm_col_offsetbyte = xpos / 8;
					U32 bm_col_offsetbit = 7 - (xpos % 8);
					U32 bit =
					!!(buffer_data[bm_row_offset
						       + bm_col_offsetbyte
					   ] & (1 << bm_col_offsetbit) );
					tmp_graydata[width*ypos + xpos] =
						255 * bit;
				}
			}
			// use newly-built graymap.
			buffer_data = tmp_graydata;
			buffer_row_stride = width;
		}

		switch (mFontBitmapCachep->getNumComponents())
		{
		case 1:
			mFontBitmapCachep->getImageRaw(bitmap_num)->setSubImage(pos_x,
																	pos_y,
																	width,
																	height,
																	buffer_data,
																	buffer_row_stride,
																	TRUE);
			break;
		case 2:
			setSubImageLuminanceAlpha(pos_x,	
									  pos_y,
									  bitmap_num,
									  width,
									  height,
									  buffer_data,
									  buffer_row_stride);
			break;
		default:
			break;
		}

		if (tmp_graydata)
			delete[] tmp_graydata;
	} else {
		// we don't know how to handle this pixel format from FreeType;
		// omit it from the font-image.
	}
	
	return TRUE;
}

BOOL LLFont::addGlyph(const llwchar wch, const U32 glyph_index) const
{
	return addGlyphFromFont(this, wch, glyph_index);
}


F32 LLFont::getXAdvance(const llwchar wch) const
{
	if (mFTFace == NULL)
		return 0.0;

	llassert(!mIsFallback);
	U32 glyph_index;

	// Return existing info only if it is current
	LLFontGlyphInfo* gi = getGlyphInfo(wch);
	if (gi && gi->mMetricsValid)
	{
		return gi->mXAdvance;
	}

	const LLFont* fontp = this;
	
	// Initialize char to glyph map
	glyph_index = FT_Get_Char_Index(mFTFace, wch);
	if (glyph_index == 0 && mFallbackFontp)
	{
		LLFontList::iterator iter;
		for(iter = mFallbackFontp->begin(); (iter != mFallbackFontp->end()) && (glyph_index == 0); iter++)
		{
			glyph_index = FT_Get_Char_Index((*iter)->mFTFace, wch);
			if(glyph_index)
			{
				fontp = *iter;
			}
		}
	}
	
	if (glyph_index)
	{
		// This font has this glyph
		fontp->renderGlyph(glyph_index);

		// Create the entry if it's not there
		char_glyph_info_map_t::iterator iter2 = mCharGlyphInfoMap.find(wch);
		if (iter2 == mCharGlyphInfoMap.end())
		{
			gi = new LLFontGlyphInfo(glyph_index);
			insertGlyphInfo(wch, gi);
		}
		else
		{
			gi = iter2->second;
		}
		
		gi->mWidth = fontp->mFTFace->glyph->bitmap.width;
		gi->mHeight = fontp->mFTFace->glyph->bitmap.rows;

		// Convert these from 26.6 units to float pixels.
		gi->mXAdvance = fontp->mFTFace->glyph->advance.x / 64.f;
		gi->mYAdvance = fontp->mFTFace->glyph->advance.y / 64.f;
		gi->mMetricsValid = TRUE;
		return gi->mXAdvance;
	}
	else
	{
		gi = get_if_there(mCharGlyphInfoMap, (llwchar)0, (LLFontGlyphInfo*)NULL);
		if (gi)
		{
			return gi->mXAdvance;
		}
	}

	// Last ditch fallback - no glyphs defined at all.
	return (F32)mFontBitmapCachep->getMaxCharWidth();
}


void LLFont::renderGlyph(const U32 glyph_index) const
{
	if (mFTFace == NULL)
		return;

	int error = FT_Load_Glyph(mFTFace, glyph_index, FT_LOAD_DEFAULT );
	llassert(!error);

	error = FT_Render_Glyph(mFTFace->glyph, gFontRenderMode);

	mRenderGlyphCount++;
	
	llassert(!error);
}


F32 LLFont::getXKerning(const llwchar char_left, const llwchar char_right) const
{
	if (mFTFace == NULL)
		return 0.0;

	llassert(!mIsFallback);
	LLFontGlyphInfo* left_glyph_info = get_if_there(mCharGlyphInfoMap, char_left, (LLFontGlyphInfo*)NULL);
	U32 left_glyph = left_glyph_info ? left_glyph_info->mGlyphIndex : 0;
	// Kern this puppy.
	LLFontGlyphInfo* right_glyph_info = get_if_there(mCharGlyphInfoMap, char_right, (LLFontGlyphInfo*)NULL);
	U32 right_glyph = right_glyph_info ? right_glyph_info->mGlyphIndex : 0;

	FT_Vector  delta;

	llverify(!FT_Get_Kerning(mFTFace, left_glyph, right_glyph, ft_kerning_unfitted, &delta));

	return delta.x*(1.f/64.f);
}

void LLFont::setSubImageLuminanceAlpha(const U32 x,
									   const U32 y,
									   const U32 bitmap_num,
									   const U32 width,
									   const U32 height,
									   const U8 *data,
									   S32 stride) const
{
	LLImageRaw *image_raw = mFontBitmapCachep->getImageRaw(bitmap_num);

	//llassert(!mIsFallback);
	llassert(image_raw && (image_raw->getComponents() == 2));

	
	U8 *target = image_raw->getData();

	if (!data)
	{
		return;
	}

	if (0 == stride)
		stride = width;

	U32 i, j;
	U32 to_offset;
	U32 from_offset;
	U32 target_width = image_raw->getWidth();
	for (i = 0; i < height; i++)
	{
		to_offset = (y + i)*target_width + x;
		from_offset = (height - 1 - i)*stride;
		for (j = 0; j < width; j++)
		{
			*(target + to_offset*2 + 1) = *(data + from_offset);
			to_offset++;
			from_offset++;
		}
	}
}
