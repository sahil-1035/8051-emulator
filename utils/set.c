#include "set.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

Set* create_set(unsigned int len)
{
	Set* new_set = (Set*) malloc( sizeof(Set) );
	new_set->arr = (int*) malloc( len * sizeof(int) );
	new_set->pointer = 0;
	new_set->max_len = len;
	return new_set;
}

void insert_set(Set* set, int val)
{
	if ( find_in_set(set, val) )
		return;

	if ( length_of_set(set) >= set->max_len )
		change_set_size(set, set->max_len * 2);

	set->arr[set->pointer] = val;
	set->pointer++;
}

void change_set_size(Set* set, unsigned int len)
{
	set->arr = (int*) realloc(set->arr, len * sizeof(int) );
	set->max_len = len;
}

char* get_str_set(Set* set, char* buff)
{
	if ( set_is_empty(set) )
		return "{}";

	sprintf(buff, "{");
	sprintf(buff, "%s %d", buff, set->arr[0]);
	for(int i = 1; i < length_of_set(set) - 1; i++)
	{
		sprintf(buff, "%s, %d", buff, set->arr[i]);
	}
	sprintf(buff, "%s }", buff);
	return buff;
}

bool find_in_set(Set* set, int val)
{
	for(int i = 0; i < length_of_set(set); i++)
	{
		if ( set->arr[i] == val )
			return true;
	}
	return false;
}

void erase_set(Set* set, int val)
{
	int val_at = -1;
	for(int i = 0; i < length_of_set(set); i++)
	{
		if ( set->arr[i] == val )
		{
			val_at = i;
			break;
		}
	}
	assert( val_at != -1 );

	memcpy(set->arr + val_at, set->arr + val_at + 1, (length_of_set(set) - val_at) * sizeof(int));
	set->pointer--;
}

int get_set_max_len(Set* set)
{
	return set->max_len;
}

void delete_set(Set* set)
{
	free(set->arr);
	free(set);
}

void clear_set(Set* set)
{
	set->pointer = 0;
}

unsigned int length_of_set(Set* set)
{
	return set->pointer + 1;
}

bool set_is_empty(Set* set)
{
	return !( (bool)set->pointer );
}
