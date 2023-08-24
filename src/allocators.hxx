// Author: Harald Servat <harald.servat@intel.com>
// Date: Feb 10, 2017
// License: To determine

#pragma once

#include <stdlib.h>

#include "allocator-statistics.hxx"
#include "allocator.hxx"

// Ajust number of allocators to the number supported external libraries
#if defined(MEMKIND_SUPPORTED) && MEMKIND_SUPPORTED
# define MEMKIND_ALLOCATORS 2
#else
# define MEMKIND_ALLOCATORS 0
#endif
#if defined(H2M_SUPPORTED) && H2M_SUPPORTED
# define H2M_ALLOCATORS 3
#else
# define H2M_ALLOCATORS 0
#endif

#define NUM_ALLOCATORS (1+MEMKIND_ALLOCATORS+H2M_ALLOCATORS)

class Allocators
{
	private:
	Allocator * allocators[NUM_ALLOCATORS+1]; // +1 for null-terminated

	public:
	Allocators (allocation_functions_t &, const char * definitions);
	~Allocators ();
	Allocator * get (const char *name);
	Allocator ** get (void);
	void show_statistics (void) const;
};
