#ifndef JOS_COLLECTIONS_H
#define JOS_COLLECTIONS_H

#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// vector

// a vector loosely modelled on std::vector
typedef struct _vector
{
	void*		_data;
	size_t		_capacity;
	size_t		_size;
	size_t		_element_size;
} vector_t;

JOS_PRIVATE_FUNC void _vector_decrease_size(vector_t *vector)
{
	JOS_ASSERT(vector->_size);
	--vector->_size;
}

JOS_PRIVATE_FUNC void _vector_increase_size(vector_t *vector)
{
	JOS_ASSERT(vector->_size<vector->_capacity);
	++vector->_size;
}

JOS_PRIVATE_FUNC void _vector_set_at(vector_t* vec, size_t i, void* element)
{
	JOS_ASSERT(vec && element && vec->_capacity && vec->_element_size && i < vec->_capacity);
	i *= vec->_element_size;
	memcpy((char*)vec->_data+i, element, vec->_element_size);
}

JOS_PRIVATE_FUNC void* _vector_at(vector_t* vec, size_t n)
{
	return (void*)((char*)vec->_data + n*vec->_element_size);
}

// create a vector able to hold capacity items of element_size
void vector_create(vector_t* vec, size_t capacity, size_t element_size)
{
	if(!vec || !capacity || !element_size)
		return;

	vec->_data = malloc(capacity * element_size);
	vec->_capacity = capacity;
	vec->_element_size = element_size;
	vec->_size = 0;
}

// add element to the end of vector
void vector_push_back(vector_t* vec, void* element)
{
	JOS_ASSERT(vec && element && vec->_capacity && vec->_element_size);

	if(vec->_size == vec->_capacity)
	{
		vec->_data = realloc(vec->_data, (vec->_capacity+=32)*vec->_element_size);
	}

	const size_t i = vec->_size*vec->_element_size;
	memcpy((char*)vec->_data+i, element, vec->_element_size);
	_vector_increase_size(vec);
}

void vector_set_at(vector_t* vec, size_t i, void* element)
{
	JOS_ASSERT(i < vec->_size);
	_vector_set_at(vec,i,element);
}

// get element at index n
void* vector_at(vector_t* vec, size_t n)
{
	JOS_ASSERT(vec && n < vec->_size);
	return _vector_at(vec,n);
}

// clear and release memory
void vector_destroy(vector_t* vec)
{
	free(vec->_data);
	memset(vec, 0, sizeof(vector_t));
}

size_t vector_size(vector_t* vec)
{
	return vec->_size;
}

size_t vector_capacity(vector_t *vec)
{
	return vec->_capacity;
}

bool vector_is_empty(vector_t* vec)
{
	return !vec->_size;
}

bool vector_is_full(vector_t* vec)
{
	return vec->_size == vec->_capacity;
}

void vector_clear(vector_t* vec)
{
	vec->_size = 0;
}

////////////////////////////////////////////////////////////////////////////////
// queue

typedef struct _queue
{
	vector_t*	_elements;
	size_t		_head;
	size_t		_tail;
} queue_t;

void queue_create(queue_t* queue, size_t capacity, size_t element_size)
{
	queue->_elements = (vector_t*)malloc(sizeof(vector_t));
	vector_create(queue->_elements, capacity, element_size);
	queue->_head = queue->_tail = 0;
}

bool queue_is_empty(queue_t* queue)
{
	return vector_is_empty(queue->_elements);
}

bool queue_is_full(queue_t* queue)
{
	return vector_is_full(queue->_elements);
}

void* queue_front(queue_t* queue)
{
	if(queue_is_empty(queue))
		return 0;
	return _vector_at(queue->_elements,queue->_head);
}

void queue_pop(queue_t* queue)
{
	if(queue_is_empty(queue))
		return;
	queue->_head = (queue->_head+1) % vector_capacity(queue->_elements);
	_vector_decrease_size(queue->_elements);
}

void queue_push(queue_t* queue, void * element)
{
	JOS_ASSERT(!queue_is_full(queue));
	_vector_set_at(queue->_elements, queue->_tail, element);
	queue->_tail = (queue->_tail+1) % vector_capacity(queue->_elements);
	_vector_increase_size(queue->_elements);
}

void queue_clear(queue_t* queue)
{
	vector_clear(queue->_elements);
	queue->_head = queue->_tail = 0;
}

void queue_destroy(queue_t* queue)
{
	vector_destroy(queue->_elements);
	free(queue->_elements);
	memset(queue,0,sizeof(queue_t));
}

#endif // JOS_COLLECTIONS_H
