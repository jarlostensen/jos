

#include "kernel_detail.h"
#include <collections.h>
#include <kernel/tasks.h>
#include "cpu_core.h"

#include <stdio.h>
#include <cpuid.h>

static const char* kCpuCoreChannel = "cpu_core";

cpu_core_context_t* _k_cpu_core_context_create(cpu_core_create_info_t* info)
{
	cpu_core_context_t* ctx = (cpu_core_context_t*)malloc(sizeof(cpu_core_context_t));
    for(size_t n = 0; n < kPri_NumLevels; ++n)
    {
        queue_create(ctx->_ready + n, 8, sizeof(task_context_t*));
		queue_create(ctx->_waiting+ n, 8, sizeof(task_context_t*));
	}
	ctx->_running = 0;
	ctx->_id = info->_id;
	ctx->_boot = info->_boot;
	// all cores start with this task running
	ctx->_running = ctx->_idle = info->_idle_task;

	//TODO: consolidate this with SMP code in cpu.c
	unsigned _eax, _ebx, _ecx, _edx;
	__cpuid(0xb, _eax, _ebx, _ecx, _edx);
	// 32 bit x2APIC for *this* CPU
	ctx->_id = _edx;
	_JOS_KTRACE_CHANNEL(kCpuCoreChannel,"context for core 0x%x\n", ctx->_id);

	return ctx;
}

bool _k_cpu_core_context_schedule(cpu_core_context_t* ctx)
{
	task_context_t* curr = ctx->_running;
	queue_t* queue = 0;
	if( !queue_is_empty(ctx->_ready+kPri_Highest) )
	{		
		queue = ctx->_ready + kPri_Highest;
	}
	else if( !queue_is_empty(ctx->_ready+kPri_Medium) )
	{
		queue = ctx->_ready + kPri_Medium;
	}
	else if( !queue_is_empty(ctx->_ready+kPri_Low) )
	{
		queue = ctx->_ready + kPri_Low;
	}
	if(queue)
	{
		// switch to the front most task
		ctx->_running = *((task_context_t**)queue_front(queue));		
		queue_pop(queue);
	}
	// ok to schedule ctx->_running now
	return( curr != ctx->_running );
}