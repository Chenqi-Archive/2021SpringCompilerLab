#pragma once


#ifdef _MSC_VER

#define ABSTRACT_BASE _declspec(novtable)
#define pure = 0

#else

#define ABSTRACT_BASE
#define pure

#endif


#include <cassert>
#include <stdexcept>


using compile_error = std::invalid_argument;


template<class T>
using ref_ptr = T*;

template<class T>
using alloc_ptr = T*;


using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int; 
using uint64 = unsigned long long;