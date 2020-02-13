#pragma once
#ifndef _JOS_H
#define _JOS_H

#ifdef __GNUC__
#define _JOS_INLINE_FUNC __attribute__((unused)) static

void _k_trace(const char* channel, const char* msg,...);
#define _JOS_KTRACE_CHANNEL(channel, msg,...) _k_trace(channel, msg,##__VA_ARGS__)
#define _JOS_KTRACE(msg,...)  _k_trace(0, msg,##__VA_ARGS__)

void _k_trace_buf(const char* channel, const void* data, size_t length);
#define _JOS_KTRACE_CHANNEL_BUF(channel, data,length) _k_trace_buf(channel, data, length)
#define _JOS_KTRACE_BUF(data,length) _k_trace_buf(0, data, length)

#define _JOS_BOCHS_DBGBREAK() asm volatile ("xchg %bx,%bx")

#ifdef _DEBUG
#define _JOS_ASSERT_COND(cond) #cond
#define _JOS_ASSERT(cond)\
if(!(cond))\
{\
    _k_trace(0, "assert %s, %s:%d\n", _JOS_ASSERT_COND(cond), __FILE__,__LINE__);\
    asm volatile ("xchg %bx,%bx");\
}
#else
#define _JOS_ASSERT(cond)
#endif

#define _JOS_ALIGN(type,name,alignment) type name __attribute__ ((aligned (alignment)))

#else
//TODO: check if this is actually VS, but we're assuming it because we're in control...

#define _JOS_INLINE_FUNC static
#define _JOS_BOCHS_DBGBREAK() __debugbreak()
#ifdef _DEBUG
#define _JOS_ASSERT(cond) if(!(cond)) { __debugbreak(); }
#else
#define _JOS_ASSERT(cond)
#endif
#define _JOS_ALIGN(type,name,alignment) __declspec(align(alignment)) type name

#endif

#ifndef min
#define min(a,b) ((a)<(b) ? (a) : (b))
#endif

#endif // _JOS_H
