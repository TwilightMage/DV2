#pragma once

/*
 * utils.h - basic utilities for (un)pack
 *
 * December 2019 Vometia a.k.a Christine Munro
 */

void *allocate(int);
char *copy(const char*);
char *finddir(char*, int);
unsigned long hash(char*);
void checkpath(char*);
void do_mkdir(char*);

template<typename T>
T* allocate(int size)
{
	return (T*)allocate(size);
}

/* vim:set syntax=c nospell tabstop=8: */
