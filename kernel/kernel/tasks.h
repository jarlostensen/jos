#ifndef JOS_TASKS_H
#define JOS_TASKS_H

typedef void (*task_func_t)(void* obj);
typedef struct _task_context 
{
    unsigned int    _id;
    unsigned int    _pri;
    void*           _stack_top;
    void*           _esp;
    void*           _ebp;
    size_t          _stack_size;
    void*           _self;    
    task_func_t     _task_func;
    void*           _obj;
} task_context_t;

void k_tasks_init(task_func_t root);
unsigned int k_task_create(unsigned int pri, task_func_t func, void* obj);

#endif // JOS_TASKS_H