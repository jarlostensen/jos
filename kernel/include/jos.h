#pragma once
#ifndef _JOS_H
#define _JOS_H

#ifdef __GNUC__
#define _JOS_PRIVATE_FUNC __attribute__((unused)) static
#define _JOS_BOCHS_DBGBREAK() asm volatile ("xchg %bx,%bx")
#ifdef _DEBUG
#define _JOS_ASSERT(cond) if(!(cond)) {asm volatile ("xchg %bx,%bx");}
#else
#define _JOS_ASSERT(cond)
#endif

#define _JOS_ALIGN(type,name,alignment) type name __attribute__ ((aligned (alignment)))

#else
//TODO: check if this is actually VS, but we're assuming it because we're in control...

#define _JOS_PRIVATE_FUNC static
#define _JOS_BOCHS_DBGBREAK() __debugbreak()
#ifdef _DEBUG
#define _JOS_ASSERT(cond) if(!(cond)) { __debugbreak(); }
#else
#define _JOS_ASSERT(cond)
#endif
#define _JOS_ALIGN(type,name,alignment) __declspec(align(alignment)) type name

#endif


#endif // _JOS_H
