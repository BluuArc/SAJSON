//
//	SAJSON.cpp
//
// Copyright (c) 2013 Natural Style Co. Ltd.
// Modified by iammathew 2022
// Additionally modified for WASM support by BluuArc 2023
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <stdio.h>
#include <assert.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "SuperAnimCommon.h"
#include "utils.h"
#include <iostream>

#define SAM_VERSION 1
#define TWIPS_PER_PIXEL (20.0f)
#define LONG_TO_FLOAT (65536.0f)

#define FRAMEFLAGS_REMOVES		0x01
#define FRAMEFLAGS_ADDS			0x02
#define FRAMEFLAGS_MOVES		0x04
#define FRAMEFLAGS_FRAME_NAME	0x08

#define MOVEFLAGS_ROTATE		0x4000
#define MOVEFLAGS_COLOR			0x2000
#define MOVEFLAGS_MATRIX		0x1000
#define MOVEFLAGS_LONGCOORDS	0x0800


#ifndef max
#define max(x,y) (((x) < (y)) ? (y) : (x))
#endif


namespace SuperAnim{
	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Animation object definition
	
	class SuperAnimObject
	{
	public:
		int mObjectNum;
		int mResNum;
		SuperAnimTransform mTransform;
		Color mColor;
	};
	
	typedef std::vector<SuperAnimObject> SuperAnimObjectVector;
	typedef std::map<int, SuperAnimObject> IntToSuperAnimObjectMap;
	
	class SuperAnimImage
	{
	public:
		SuperAnimSpriteId mSpriteId;
		std::string mImageName;
		int mWidth;
		int mHeight;
		SuperAnimTransform mTransform;
	};
	typedef std::vector<SuperAnimImage> SuperAnimImageVector;
	
	class SuperAnimFrame
	{
	public:
		SuperAnimObjectVector mObjectVector;
	};
	typedef std::vector<SuperAnimFrame> SuperAnimFrameVector;
	typedef std::map<std::string, int> StringToIntMap;
	class SuperAnimLabel{
	public:
		std::string mLabelName;
		int mStartFrameNum;
		int mEndFrameNum;
	};
	typedef std::vector<SuperAnimLabel> SuperAnimLabelArray;
	class SuperAnimMainDef
	{
	public:
		SuperAnimFrameVector mFrames;
		int mStartFrameNum;
		int mEndFrameNum;
		int mAnimRate;
		SuperAnimLabelArray mLabels;
		int mX;
		int mY;
		int mWidth;
		int mHeight;
		SuperAnimImageVector mImageVector;
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////

	typedef std::map<std::string, SuperAnimMainDef> SuperAnimMainDefMap;
	class SuperAnimDefMgr
	{
	private:
		SuperAnimMainDefMap mMainDefCache;
	private:
		SuperAnimDefMgr();
		~SuperAnimDefMgr();
		
		// std::string theSuperAnimFile include the absolute path
		bool LoadSuperAnimMainDef(const std::string &theSuperAnimFile, bool effectFile = false);
	public:
		static SuperAnimDefMgr *GetInstance();
		static void DestroyInstance();

		// std::string theSuperAnimFile include the absolute path
		SuperAnimMainDef *Load_GetSuperAnimMainDef(const std::string &theSuperAnimFile, bool effectFile = false);
		void UnloadSuperAnimMainDef(const std::string &theName);
	};


	SuperAnimSpriteId LoadSuperAnimSprite(std::string theSpriteName){
		return InvalidSuperAnimSpriteId;
	}
	
	void UnloadSuperSprite(SuperAnimSpriteId theSpriteId){
		// none
	}
}

emscripten_fetch_t *lastFetch = NULL;

unsigned char* GetFileData(const char* pszFileName, const char* pszMode, unsigned long * pSize) {
	lastFetch = getPrefetchedUrl();
	*pSize = lastFetch->numBytes;
	std::cout << "Using prefetched data of size " << lastFetch->numBytes << std::endl;

	return (unsigned char *) lastFetch->data;
}

std::string createKeyValueJsonString(const char* key, const char* value, bool is_end = false) {
	std::string result = "\"";
	result.append(key);
	result.append("\":");
	result.append(value);
	if (!is_end) {
		result.append(",");
	}
	return result;
}

template <typename T>
std::string createKeyValueJsonStringForNumber(const char* key, T& value, bool is_end = false) {
	return createKeyValueJsonString(key, std::to_string(value).c_str(), is_end);
}

std::string createKeyValueJsonStringForString(const char* key, const char* value, bool is_end = false) {
	std::string quotedValue = "\"";
	quotedValue.append(value);
	quotedValue.append("\"");
	return createKeyValueJsonString(key, quotedValue.c_str(), is_end);
}

std::string convert3x3MatrixToJsonString(const float matrix[3][3]) {
	std::string result = "[";
	for (int i = 0; i < 3; ++i) {
		result.append("[");
		for (int j = 0; j < 3; ++j) {
			result.append(std::to_string(matrix[i][j]));
			if (j != 2) {
				result.append(",");
			}
		}
		if (i != 2) {
			result.append("],");
		} else {
			result.append("]");
		}
	}
	result.append("]");
	return result;
}

char* getSamJsonString(bool effect){
	// expect that prefetchUrl was called beforehand to preload the data into memory
	std::string fakeFile = "fake_file.sam";
	SuperAnim::SuperAnimMainDef* p = SuperAnim::SuperAnimDefMgr::GetInstance()->Load_GetSuperAnimMainDef(fakeFile, effect);
	std::cout << "converting to JSON" << std::endl;

	std::string result;
	result.append("{");

	result.append(createKeyValueJsonStringForNumber("mAnimRate", p->mAnimRate));
	result.append(createKeyValueJsonStringForNumber("mX", p->mX));
	result.append(createKeyValueJsonStringForNumber("mY", p->mY));
	result.append(createKeyValueJsonStringForNumber("mWidth", p->mWidth));
	result.append(createKeyValueJsonStringForNumber("mHeight", p->mHeight));

	std::cout << "converting mImageVector of size " << p->mImageVector.size() << std::endl;
	result.append("\"mImageVector\":[");
	bool first = true;
	for(SuperAnim::SuperAnimImageVector::const_iterator i=p->mImageVector.begin(); i!=p->mImageVector.end(); ++i){
		if (first) {
			first = false;
		} else {
			result.append(",");
		}
		result.append("{");
		result.append(createKeyValueJsonStringForString("mImageName", i->mImageName.c_str()));
		result.append(createKeyValueJsonStringForNumber("mWidth", i->mWidth));
		result.append(createKeyValueJsonStringForNumber("mHeight", i->mHeight));

		result.append("\"mTransform\":{\"mMatrix\":{");
		result.append(createKeyValueJsonString(
			"m",
			convert3x3MatrixToJsonString(i->mTransform.mMatrix.m).c_str(),
			true
		));
		result.append("}}");

		result.append("}");
	}
	result.append("],");

	result.append(createKeyValueJsonStringForNumber("mStartFrameNum", p->mStartFrameNum));
	result.append(createKeyValueJsonStringForNumber("mEndFrameNum", p->mEndFrameNum));

	std::cout << "converting mFrames of size " << p->mFrames.size() << std::endl;
	bool firstFrame = true;
	result.append("\"mFrames\":[");
	for(SuperAnim::SuperAnimFrameVector::const_iterator i=p->mFrames.begin(); i!=p->mFrames.end(); ++i){
		if (firstFrame) {
			firstFrame = false;
		} else {
			result.append(",");
		}
		bool firstObject = true;
		result.append("{ \"mObjectVector\":[");
		for(SuperAnim::SuperAnimObjectVector::const_iterator j=i->mObjectVector.begin(); j!=i->mObjectVector.end(); ++j){
			if (firstObject) {
				firstObject = false;
			} else {
				result.append(",");
			}
			result.append("{");
			result.append(createKeyValueJsonStringForNumber("mObjectNum", j->mObjectNum));
			result.append(createKeyValueJsonStringForNumber("mResNum", j->mResNum));
			result.append("\"mTransform\":{\"mMatrix\":{");
			result.append(createKeyValueJsonString(
				"m",
				convert3x3MatrixToJsonString(j->mTransform.mMatrix.m).c_str(),
				true
			));
			result.append("}},\"mColor\":{");
			result.append(createKeyValueJsonStringForNumber("mRed", j->mColor.mRed));
			result.append(createKeyValueJsonStringForNumber("mGreen", j->mColor.mGreen));
			result.append(createKeyValueJsonStringForNumber("mBlue", j->mColor.mBlue));
			result.append(createKeyValueJsonStringForNumber("mAlpha", j->mColor.mAlpha, true));
			result.append("}}");
		}
		result.append("]}");
	}
	result.append("],");
	std::cout << "converting mLabels of size " << p->mLabels.size() << std::endl;
	bool firstLabel = true;
	result.append("\"mLabels\":[");
	for(SuperAnim::SuperAnimLabelArray::const_iterator i=p->mLabels.begin(); i!=p->mLabels.end(); ++i){
		if (firstLabel) {
			firstLabel = false;
		} else {
			result.append(",");
		}
		result.append("{");
		result.append(createKeyValueJsonStringForString("mLabelName", i->mLabelName.c_str()));
		result.append(createKeyValueJsonStringForNumber("mStartFrameNum", i->mStartFrameNum));
		result.append(createKeyValueJsonStringForNumber("mEndFrameNum", i->mEndFrameNum, true));
		result.append("}");
	}
	result.append("]");

	result.append("}");

	lastFetch = NULL;
	SuperAnim::SuperAnimDefMgr::GetInstance()->UnloadSuperAnimMainDef(fakeFile);
	return (char*) result.c_str();
}

extern "C" {
	int prefetch_url(char* url) {
		prefetchUrl(url);
		return 0;
	}
	char* get_sam_json_string(bool effect) {
		return getSamJsonString(effect);
	}
	int clear_prefetched_data() {
		clearPrefetchedData();
		return 0;
	}
}