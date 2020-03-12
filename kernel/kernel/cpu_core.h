#ifndef _JOS_CPU_CORE_H
#define _JOS_CPU_CORE_H

enum 
{
	kPri_Idle = 0,
	kPri_Low,
	kPri_Medium,
	kPri_Highest,

	kPri_NumLevels
};

// initialisation info for a CPU core context
typedef struct _cpu_core_create_info
{
	// x2APIC id
	unsigned int _id;
    task_context_t* _idle_task;
	bool		 _boot;

} cpu_core_create_info_t;

// we associate one of these with each core and use it for scheduling etc.
typedef struct _cpu_core_context
{	
	// tasks ready to run, in order of priority
	queue_t _ready[kPri_NumLevels];
	// tasks waiting for something, in order of priority
	queue_t _waiting[kPri_NumLevels];
	// currently running task (only ever one per core, obviously)
	task_context_t* _running;
    // the context of the idle task for this core
    task_context_t* _idle;
	// x2APIC ID
	unsigned int	_id;
	int				_boot:1;

	//TODO: these things are really all "perf counters"
	size_t			_task_switches;

} cpu_core_context_t;

// create a context (for the current CPU...perhaps?)
cpu_core_context_t* _k_cpu_core_context_create(cpu_core_create_info_t* info);
// schedule the next task on this CPU according to some implementation
// TODO: make this data driven and dynamic, ideally, so that we can play with schedulers in real time
bool _k_cpu_core_context_schedule(cpu_core_context_t* ctx);

#endif // _JOS_CPU_CORE_H