// Author: Clément Foyer <clement.foyer@univ-reims.fr>
// Date: Jul 06, 2023
// License: To determine

#pragma once

#include "allocator.hxx"
#include "allocator-statistics.hxx"

class StatisticsRecorderAllocator : public Allocator
{
	protected:
	AllocatorStatistics _stats;

	public:
	StatisticsRecorderAllocator (allocation_functions_t &af)
	  : Allocator(af) {}
	~StatisticsRecorderAllocator() = default;

	void show_statistics (void) const
	  { _stats.show_statistics (this->name(), true); }

	size_t hwm (void) const
	  { return _stats.water_mark(); }
	bool fits (size_t s) const
	  { return this->hwm() + s <= this->size(); }

	void record_unfitted_malloc (size_t s)
	  { _stats.record_unfitted_malloc (s); } ;
	void record_unfitted_calloc (size_t s)
	  { _stats.record_unfitted_calloc (s); } ;
	void record_unfitted_aligned_malloc (size_t s)
	  { _stats.record_unfitted_aligned_malloc (s); } ;
	void record_unfitted_realloc (size_t s)
	  { _stats.record_unfitted_realloc (s); } ;

	void record_source_realloc (size_t s)
	  { _stats.record_source_realloc (s); };
	void record_target_realloc (size_t s)
	  { _stats.record_target_realloc (s); };
	void record_self_realloc (size_t s)
	  { _stats.record_self_realloc (s); };

	void record_realloc_forward_malloc (void)
	  { _stats.record_realloc_forward_malloc (); }
};
