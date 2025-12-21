#pragma once
#include <iostream>

typedef char                int8;
typedef unsigned char       uint8;
typedef short               int16;
typedef unsigned short      uint16;
typedef int                 int32;
typedef unsigned int        uint32;
typedef long long           int64;
typedef unsigned long long  uint64;


// These macros must exactly match those in the Windows SDK's intsafe.h.
#ifndef INT8_MIN
#define INT8_MIN         (-127i8 - 1)
#endif
#ifndef INT16_MIN
#define INT16_MIN        (-32767i16 - 1)
#endif
#ifndef INT32_MIN
#define INT32_MIN        (-2147483647i32 - 1)
#endif
#ifndef INT64_MIN
#define INT64_MIN        (-9223372036854775807i64 - 1)
#endif
#ifndef INT8_MAX
#define INT8_MAX         127i8
#endif
#ifndef INT16_MAX
#define INT16_MAX        32767i16
#endif
#ifndef INT32_MAX
#define INT32_MAX        2147483647i32
#endif
#ifndef INT64_MAX
#define INT64_MAX        9223372036854775807i64
#endif
#ifndef UINT8_MAX
#define UINT8_MAX        0xffui8
#endif
#ifndef UINT16_MAX
#define UINT16_MAX       0xffffui16
#endif
#ifndef UINT32_MAX
#define UINT32_MAX       0xffffffffu
#endif
#ifndef UINT64_MAX
#define UINT64_MAX       0xffffffffffffffffull
#endif
#ifndef FLOAT_MAX
#define FLOAT_MAX        3.402823466e+38f
#endif
#ifndef FLOAT_MIN
#define FLOAT_MIN        (-FLOAT_MAX)
#endif

#define INVALID_INDEX_U32 UINT32_MAX

#define XX_NODISCARD [[nodiscard]]

#define MoveTemp(x) (std::move(x))


#define NON_COPYABLE(cls)\
    cls(const cls&) = delete;\
    cls& operator=(const cls&) = delete

#define NON_MOVEABLE(cls)\
    cls(cls&&) noexcept = delete; \
    cls& operator=(cls&&)noexcept = delete

#define SINGLETON_INSTANCE(cls)\
public:\
    static void Release(){ if(s_Instance) delete s_Instance; }\
    template <class ...Args> static void Initialize(Args...args) { Release(); s_Instance = new cls(args...); }\
    static cls* Instance() {return s_Instance; }\
private:\
    NON_COPYABLE(cls);\
    NON_MOVEABLE(cls);\
    inline static cls* s_Instance {nullptr}
