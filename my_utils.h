#pragma once
#include <stdio.h>
namespace my_utils {

	constexpr auto INVALID_NUMBER = -1;

//************ASSIMP CONSTANTS********************//
#define AI_MATKEY_COLOR_AMBIENT_str__ "AI_MATKEY_COLOR_AMBIENT"
#define AI_MATKEY_COLOR_DIFFUSE_str__ "AI_MATKEY_COLOR_DIFFUSE"

//default values taken from ColladaHelper.h (see "Effect()" constructor)
#define IS_DEFAULT_4D_COLOR(__ptr_to_aiColor4D, __MAT_TYPE_str, _ptr_boolean_out) \
{ \
	if(strcmp(__MAT_TYPE_str, AI_MATKEY_COLOR_AMBIENT_str__) == 0) \
	{\
		*(_ptr_boolean_out) = (__ptr_to_aiColor4D->r == 0.1f) && (__ptr_to_aiColor4D->g == 0.1f) && (__ptr_to_aiColor4D->b == 0.1f) && (__ptr_to_aiColor4D->a == 1.0f);\
	} else if(strcmp(__MAT_TYPE_str, AI_MATKEY_COLOR_DIFFUSE_str__) == 0){ \
		*(_ptr_boolean_out) = (__ptr_to_aiColor4D->r == 0.6f) && (__ptr_to_aiColor4D->g == 0.6f) && (__ptr_to_aiColor4D->b == 0.6f) && (__ptr_to_aiColor4D->a == 1.0f);\
	} else { \
		_STL_ASSERT(__MAT_TYPE_str == NULL, "unknwon material type for IS_DEFAULT_4D_COLOR"); \
	}\
}
	

//************ARRAY OPERATIONS********************//
#define DELETE_PTR(__ptr__) \
if (__ptr__ != NULL) \
{ \
	delete __ptr__; \
	__ptr__ = NULL; \
}

#define DELETE_ARR(__arr__) \
if(__arr__ != NULL) \
{ \
	delete [] __arr__; \
	__arr__ = NULL; \
}

#define COPY_CHAR_ARRAYS(__source_, __source_start_index, __destination, __destination_start_index, __length) \
for (int i = 0; i < __length; i++) \
{ \
	__destination[i+__destination_start_index] = __source_[i+__source_start_index];\
}

#define CHAR_ARRAY_TO_INT(__source_, __source_start_index, __source_end_index, __ptr_int_out) \
{ \
	const int SIZE = __source_end_index - __source_start_index; \
	char* pTmp = new char[SIZE + 1]; \
	pTmp[SIZE] = NULL; \
	for (int i = 0; i < SIZE; i++) \
	{ \
		pTmp[i] = __source_[i + __source_start_index]; \
	} \
	*(__ptr_int_out) = strtol(pTmp, NULL, 10); \
	DELETE_ARR(pTmp); \
}

}
