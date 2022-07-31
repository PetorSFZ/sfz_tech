// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "sfz/SfzScheduler.h"

#include <skipifzero_arrays.hpp>
#include <skipifzero_new.hpp>

#include "sfz/SfzContext.h"
#include "sfz/SfzTask.h"

// Types
// ------------------------------------------------------------------------------------------------

sfz_struct(SfzScheduler) {
	SfzAllocator* allocator;
	sfz::Array<SfzTask> tasks;
};

// Scheduler
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C SfzScheduler* sfzSchedulerCreate(SfzAllocator* allocator)
{
	SfzScheduler* scheduler = sfz_new<SfzScheduler>(allocator, sfz_dbg("SfzScheduler"));
	scheduler->allocator = allocator;
	constexpr u32 DEFAULT_TASKS_CAPACITY = 1024;
	scheduler->tasks.init(DEFAULT_TASKS_CAPACITY, allocator, sfz_dbg("SfzScheduler::tasks"));
	return scheduler;
}

SFZ_EXTERN_C void sfzSchedulerDestroy(SfzScheduler* scheduler)
{
	if (scheduler == nullptr) return;
	SfzAllocator* allocator = scheduler->allocator;
	sfz_delete(allocator, scheduler);
}

SFZ_EXTERN_C void sfzSchedulerScheduleTask(SfzScheduler* scheduler, const SfzTask* task)
{
	sfz_assert(task->taskFunc != nullptr);
	sfz_assert((task->readAccess & task->writeAccess) == task->writeAccess);
	scheduler->tasks.add(*task);
}

SFZ_EXTERN_C void sfzSchedulerRunTasks(SfzScheduler* scheduler, SfzCtx* ctx)
{
	const u32 numTasks = scheduler->tasks.size();
	for (u32 i = 0; i < numTasks; i++) {
		const SfzTask& task = scheduler->tasks[i];
		SfzCtxView* view = sfzCtxCreateView(ctx, task.readAccess, task.writeAccess);
		sfz_assert(task.taskFunc != nullptr);
		task.taskFunc(view);
		sfzCtxDestroyView(ctx, view);
	}
}
