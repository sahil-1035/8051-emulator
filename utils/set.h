#ifndef SET_H
#define SET_H

#include <stdbool.h>

typedef struct Set {
	int* arr;
	int pointer;
	int max_len;
} Set;

Set* create_set(unsigned int len);
void delete_set(Set* set);
void change_set_size(Set* set, unsigned int len);

void insert_set(Set* set, int val);
void erase_set(Set* set, int val);

bool find_in_set(Set* set, int val);

int get_set_max_len(Set* set);
char* get_str_set(Set* set, char* buff);
bool set_is_empty(Set* set);
void clear_set(Set* set);
unsigned int length_of_set(Set* set);
#endif
