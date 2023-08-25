// Author: Harald Servat <harald.servat@intel.com>
// Author: Clément Foyer <clement.foyer@univ-reims.fr>
// Date: Jul 04, 2023
// License: To determine

#include <stdlib.h>
#include <h2m.h>
#include <h2m_tools.h>
#include <h2m_common.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "common.hxx"
#include "allocator-h2m.hxx"

#define ALLOCATOR_NAME "h2m"
#define MAKE_ALLOCATOR_NAME(name) ALLOCATOR_NAME "/" name

#define BW_NAME         MAKE_ALLOCATOR_NAME("bandwidth")
#define LAT_NAME        MAKE_ALLOCATOR_NAME("latency")
#define LARGE_CAP_NAME  MAKE_ALLOCATOR_NAME("large_cap")

AllocatorH2mAlloc::AllocatorH2mAlloc (allocation_functions_t &af)
	: StatisticsRecorderAllocator (af)
{
	int any_loc = h2m_atv_mem_space_hbw | h2m_atv_mem_space_low_lat | h2m_atv_mem_space_large_cap;
	_traits[0] = { h2m_atk_req_mem_space, any_loc };
	_traits[1] = { h2m_atk_mem_alignment, 1 };
	usage_info = h2m_get_capacity_usage_info();
}

AllocatorH2mAllocBandwidth::AllocatorH2mAllocBandwidth (allocation_functions_t &af)
	: AllocatorH2mAlloc (af)
{
	setMemSpace(h2m_atv_mem_space_hbw);
}

AllocatorH2mAllocLatency::AllocatorH2mAllocLatency (allocation_functions_t &af)
	: AllocatorH2mAlloc (af)
{
	setMemSpace(h2m_atv_mem_space_low_lat);
}

AllocatorH2mAllocLargeCap::AllocatorH2mAllocLargeCap (allocation_functions_t &af)
	: AllocatorH2mAlloc (af)
{
	setMemSpace(h2m_atv_mem_space_large_cap);
}

AllocatorH2mAlloc::~AllocatorH2mAlloc ()
{
}

AllocatorH2mAllocBandwidth::~AllocatorH2mAllocBandwidth ()
{
}

AllocatorH2mAllocLatency::~AllocatorH2mAllocLatency ()
{
}

AllocatorH2mAllocLargeCap::~AllocatorH2mAllocLargeCap ()
{
}

void AllocatorH2mAlloc::setAlignment(size_t align)
{
	setAlignment(_traits, align);
}

void AllocatorH2mAlloc::setAlignment(h2m_alloc_trait_t traits[], size_t align)
{
	traits[1].value.sz = align;
}

void AllocatorH2mAlloc::setMemSpace(h2m_alloc_trait_value_t mem_space)
{
	setMemSpace(_traits, mem_space);
}

void AllocatorH2mAlloc::setMemSpace(h2m_alloc_trait_t traits[], h2m_alloc_trait_value_t mem_space)
{
	traits[0].value.atv = mem_space;
}

void * AllocatorH2mAlloc::malloc (size_t size)
{
	int err;
	h2m_alloc_trait_t traits[N_TRAITS];
	memcpy(traits, _traits, sizeof(traits));
	// Forward memory request to real malloc and reserve some space for the header
	void * baseptr = h2m_alloc_w_traits (Allocator::getTotalSize (size), &err, 1, traits);
	void * res = nullptr;

	// If malloc succeded, then forge a header and the pointer points to the
	// data space after the header
	if (H2M_SUCCESS == err && baseptr)
	{
		res = Allocator::generateAllocatorHeader (baseptr, this, size);

		// Verbosity and emit statistics
		VERBOSE_MSG(3, ALLOCATOR_NAME": Allocated %lu bytes in %p (hdr & base at %p) w/ allocator %s (%p)\n", size, res, Allocator::getAllocatorHeader (res), name(), this);
		_stats.record_malloc (size);
	}

	return res;
}

void * AllocatorH2mAlloc::calloc (size_t nmemb, size_t size)
{
	int err;
	h2m_alloc_trait_t traits[N_TRAITS];
	memcpy(traits, _traits, sizeof(traits));
	// Forward memory request to real malloc and request additional space to store
	// the allocator and the basepointer
	void * baseptr = h2m_alloc_w_traits (Allocator::getTotalSize (nmemb * size), &err, 1, traits);
	void * res = nullptr;

	// If malloc succeded, then forge a header and the pointer points to the
	// data space after the header
	if (H2M_SUCCESS == err && baseptr)
	{
		res = Allocator::generateAllocatorHeader (baseptr, this, nmemb * size);

		// Verbosity and emit statistics
		VERBOSE_MSG(3, ALLOCATOR_NAME": Allocated %lu bytes in %p (hdr & base %p) w/ allocator %s (%p)\n", size, res, Allocator::getAllocatorHeader (res), name(), this);
		_stats.record_calloc (nmemb * size);
	}

	return res;
}

int AllocatorH2mAlloc::posix_memalign (void **ptr, size_t align, size_t size)
{
	assert (ptr != nullptr);

	int err;
	h2m_alloc_trait_t traits[N_TRAITS];
	memcpy(traits, _traits, sizeof(traits));
	// Forward memory request to real malloc and request additional space to
	// store the allocator and the basepointer
	setAlignment(align);
	void * baseptr = h2m_alloc_w_traits (Allocator::getTotalSize (size + align), &err, 2, traits);
	void * res = nullptr;

	// If malloc succeded, then forge a header and the pointer points to the
	// data space after the header
	if (H2M_SUCCESS == err && baseptr)
	{
		res = Allocator::generateAllocatorHeaderOnAligned (baseptr, align, this, size);

		// Verbosity and emit statistics
		VERBOSE_MSG(3, ALLOCATOR_NAME": Allocated %lu bytes in %p (hdr %p, base %p) w/ allocator %s (%p)\n", size, res, Allocator::getAllocatorHeader (res), baseptr, name(), this);
		_stats.record_aligned_malloc (size + align);

		*ptr = res;
		return 0;
	}
	else
		return ENOMEM;
}

void AllocatorH2mAlloc::free (void *ptr)
{
	Allocator::Header_t *hdr = Allocator::getAllocatorHeader (ptr);

	// When freeing the memory, need to free the base pointe
	VERBOSE_MSG(3, ALLOCATOR_NAME": Freeing up pointer %p (hdr %p) w/ size - %lu (base pointer located in %p)\n", ptr, hdr, hdr->size, hdr->base_ptr);

	_stats.record_free (hdr->size);
	h2m_free (hdr->base_ptr);
}

void * AllocatorH2mAlloc::realloc (void *ptr, size_t size)
{
	// If previous pointer is not null, behave normally. otherwise, behave like a malloc but
	// without calling information
	if (ptr)
	{
		// Search for previous allocation size through the header
		Allocator::Header_t *prev_hdr = Allocator::getAllocatorHeader (ptr);
		size_t prev_size = prev_hdr->size;
		void * prev_baseptr = prev_hdr->base_ptr;
		uintptr_t extra_size = Allocator::getExtraSize (prev_hdr);

		if (prev_size < size)
		{
			int err;
			h2m_alloc_trait_t traits[N_TRAITS];
			memcpy(traits, _traits, sizeof(traits));
			// Reallocate, from base pointer to fit the new size plus a new header
			void *new_baseptr = h2m_alloc_w_traits (Allocator::getTotalSize (size + extra_size), &err, 1, traits);
			void *res = nullptr;

			// We need to update the header.
			if (H2M_SUCCESS == err && new_baseptr)
			{
				memcpy (new_baseptr, prev_baseptr, prev_size);
				h2m_free(prev_baseptr);
				// res points to the space where the user can store their data
				res = Allocator::generateAllocatorHeader (new_baseptr, extra_size, this, size);
				DBG("Reallocated (%ld->%ld [extra bytes = %lu]) from %p (base at %p, header at %p) into %p (base at %p, header at %p) w/ allocator %s (%p)\n", prev_size, size, extra_size, ptr, prev_baseptr, prev_hdr, res, new_baseptr, Allocator::getAllocatorHeader (res), name(), this);
			}

			_stats.record_realloc (size, prev_size);

			return res;
		}
		else
		{
			DBG("Reallocated (%ld->%ld) from %p but not touching as new size is smaller w/ allocator %s (%p)\n", prev_size, size, ptr, name(), this);
			return ptr;
		}
	}
	else
	{
		VERBOSE_MSG(3, ALLOCATOR_NAME": realloc(NULL, ...) forwarded to malloc\n");
		_stats.record_realloc_forward_malloc();

		return this->malloc (size);
	}
}

size_t AllocatorH2mAlloc::malloc_usable_size (void *ptr)
{
	Allocator::Header_t *hdr = Allocator::getAllocatorHeader (ptr);

	// When checking for the usable size, return the size we requested originally, no matter
	// what the underlying library did. This may alter execution behaviors, though.
	VERBOSE_MSG(3, ALLOCATOR_NAME": Checking usable size on pointer %p w/ size - %lu (but base pointer located in %p)\n",
		ptr, hdr->size, hdr->base_ptr);

	return hdr->size;
}

template <const char *allocator_name>
struct allocator_fn_wrapper {
	static inline void configure (Allocator& alloc, const char *config)
	{
		const char * MEMORYCONFIG_SIZE = "Size ";
		const char * MEMORYCONFIG_MBYTES_SUFFIX = " MBytes";

		if (strncmp (config, MEMORYCONFIG_SIZE, strlen(MEMORYCONFIG_SIZE)) == 0)
		{
			// Get given size after the Size marker
			char *pEnd = nullptr;
			long long s_size = strtoll (&config[strlen(MEMORYCONFIG_SIZE)], &pEnd, 10);
			// Was text converted into s? If so, now look for suffix
			if (pEnd != &config[strlen(MEMORYCONFIG_SIZE)])
			{
				if (strncmp (pEnd, MEMORYCONFIG_MBYTES_SUFFIX, strlen(MEMORYCONFIG_MBYTES_SUFFIX)) == 0)
				{
					size_t s = ((size_t) s_size) << 20;
					if (s_size < 0)
					{
						VERBOSE_MSG(1, "%s: Invalid given size.\n", allocator_fn_wrapper::name());
						exit (1);
					}
					else if (s > alloc.size())
					{
						VERBOSE_MSG(1, "%s: Invalid given size, it cannot exceed the memory available (%zu MiB)\n", allocator_fn_wrapper::name(), alloc.size() >> 20);
						exit (1);
					}
					VERBOSE_MSG(1, "%s: Setting up size %lld MBytes.\n", allocator_fn_wrapper::name(), s_size);
					alloc.size (s);
				}
				else
				{
					VERBOSE_MSG(0, "%s: Invalid size suffix.\n", allocator_fn_wrapper::name());
					exit (1);
				}
			}
			else
			{
				VERBOSE_MSG(0, "%s: Could not parse given size.\n", allocator_fn_wrapper::name());
				exit (1);
			}
		}
		else
		{
			// No need to configure anything.
			// The configuration line may be used to restrict the available memory.
		}
	}

	static inline const char * name (void)
	{
		return allocator_name;
	}

	static inline const char * description (void)
	{
		return "Allocator based on H2M library.";
	}
};

inline namespace { constexpr char bw_alloc_name[] = BW_NAME; }
void AllocatorH2mAllocBandwidth::configure (const char* config)
{
	size (usage_info.max_mem_cap_bytes_hbw);
	allocator_fn_wrapper<bw_alloc_name>::configure (*this, config);
	_is_ready = true;
}
const char * AllocatorH2mAllocBandwidth::name (void) const
{
	return allocator_fn_wrapper<bw_alloc_name>::name();
}
const char * AllocatorH2mAllocBandwidth::description (void) const
{
	return allocator_fn_wrapper<bw_alloc_name>::description();
}

inline namespace { constexpr char lat_alloc_name[] = LAT_NAME; }
void AllocatorH2mAllocLatency::configure (const char* config)
{
	size (usage_info.max_mem_cap_bytes_low_lat);
	allocator_fn_wrapper<lat_alloc_name>::configure (*this, config);
	_is_ready = true;
}
const char * AllocatorH2mAllocLatency::name (void) const
{
	return allocator_fn_wrapper<lat_alloc_name>::name();
}
const char * AllocatorH2mAllocLatency::description (void) const
{
	return allocator_fn_wrapper<lat_alloc_name>::description();
}

inline namespace { constexpr char large_cap_alloc_name[] = LARGE_CAP_NAME; }
void AllocatorH2mAllocLargeCap::configure (const char* config)
{
	size (usage_info.max_mem_cap_bytes_large_cap);
	allocator_fn_wrapper<large_cap_alloc_name>::configure (*this, config);
	_is_ready = true;
}
const char * AllocatorH2mAllocLargeCap::name (void) const
{
	return allocator_fn_wrapper<large_cap_alloc_name>::name();
}
const char * AllocatorH2mAllocLargeCap::description (void) const
{
	return allocator_fn_wrapper<large_cap_alloc_name>::description();
}

