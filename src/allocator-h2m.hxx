// Author: Harald Servat <harald.servat@intel.com>
// Author: Clément Foyer <clement.foyer@univ-reims.fr>
// Date: Jul 04, 2023
// License: To determine

#pragma once

#include "statistics-recorder-allocator.hxx"
#include <h2m.h>

class AllocatorH2mAlloc : public StatisticsRecorderAllocator
{
	protected:
	static constexpr size_t N_TRAITS = 2;

	private:
	h2m_alloc_trait_t _traits[N_TRAITS];	// Placement + potential alignment

	protected:
	void setAlignment(size_t);
	static void setAlignment(h2m_alloc_trait_t [], size_t);
	void setMemSpace(h2m_alloc_trait_value_t);
	static void setMemSpace(h2m_alloc_trait_t [], h2m_alloc_trait_value_t);

	public:
	AllocatorH2mAlloc (allocation_functions_t &);
	~AllocatorH2mAlloc() = 0;

	void*  malloc (size_t);
	void*  calloc (size_t, size_t);
	int    posix_memalign (void **, size_t, size_t);
	void   free (void *);
	void*  realloc (void *, size_t);
	size_t malloc_usable_size (void*);
};

class AllocatorH2mAllocBandwidth final : public AllocatorH2mAlloc
{
	public:
	AllocatorH2mAllocBandwidth (allocation_functions_t &);
	~AllocatorH2mAllocBandwidth();

	void   configure (const char *);
	const char * name (void) const;
	const char * description (void) const;
};

class AllocatorH2mAllocLatency final : public AllocatorH2mAlloc
{
	public:
	AllocatorH2mAllocLatency (allocation_functions_t &);
	~AllocatorH2mAllocLatency();

	void   configure (const char *);
	const char * name (void) const;
	const char * description (void) const;
};

class AllocatorH2mAllocLargeCap final : public AllocatorH2mAlloc
{
	public:
	AllocatorH2mAllocLargeCap (allocation_functions_t &);
	~AllocatorH2mAllocLargeCap();

	void   configure (const char *);
	const char * name (void) const;
	const char * description (void) const;
};

