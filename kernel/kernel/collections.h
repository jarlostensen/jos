#ifndef JOS_COLLECTIONS_H
#define JOS_COLLECTIONS_H

#include <stdlib.h>
#include <string.h>

// a vector loosely modelled on std::vector
typedef struct _vector
{
	void*		_data;
	size_t		_capacity;
	size_t		_size;
	size_t		_element_size;
} vector_t;

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
bool vector_push_back(vector_t* vec, void* element)
{
	if(!vec || !element || !vec->_capacity || !vec->_element_size)
		return false;

	if(vec->_size == vec->_capacity)
	{
		vec->_data = realloc(vec->_data, (vec->_capacity+=32)*vec->_element_size);
	}

	const size_t i = vec->_size*vec->_element_size;
	memcpy((char*)vec->_data+i, element, vec->_element_size);
	++vec->_size;
	return true;
}

// get element at index n
void* vector_at(vector_t* vec, size_t n)
{
	if(!vec || n >= vec->_size)
		return 0;

	return (void*)((char*)vec->_data + n*vec->_element_size);
}

// clear and release memory
void vector_destroy(vector_t* vec)
{
	free(vec->_data);
	memset(vec, 0, sizeof(vector_t));
}

bool vector_is_empty(vector_t* vec)
{
	return !vec->_size;
}

bool vector_is_full(vector_t* vec)
{
	return vec->_size == vec->_capacity;
}


#endif // JOS_COLLECTIONS_H
