#pragma once
#ifndef JOS_H
#define JOS_H

#ifdef __GNUC__
#define JOS_PRIVATE_FUNC __attribute__((unused)) static
#define JOS_BOCHS_DBGBREAK() asm volatile ("xchg %bx,%bx")
#define JOS_ASSERT(cond) if(!(cond)) {asm volatile ("xchg %bx,%bx");}

#else
//TODO: check if this is actually VS, but we're assuming it because we're in control...

#define JOS_PRIVATE_FUNC static
#define JOS_BOCHS_DBGBREAK() __debugbreak()
#define JOS_ASSERT(cond) if(!(cond)) { __debugbreak(); }

#endif


#endif // JOS_H
