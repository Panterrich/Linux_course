#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "list_pointer.h"

struct hashtable
{
    struct list_pointer* keys;
    size_t size;
};

int          hashtable_ctor(     struct hashtable* table, size_t size);
int          hashtable_dtor(     struct hashtable* table);
int          hashtable_get_value(struct hashtable* table, const char* key);
unsigned int hashtable_set_value(struct hashtable* table, const char* key, int data);
void         hashtable_remove(   struct hashtable* table, const char* key);
void         hashtable_clear(    struct hashtable* table);


unsigned int MurmurHash2(const char* key);


#endif