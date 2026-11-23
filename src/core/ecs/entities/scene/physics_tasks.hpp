#pragma once

#include "box2d/types.h"

namespace atmo::core::ecs::physics
{
    void *EnqueueTask(b2TaskCallback *task, int item_count, int min_range, void *task_context, void *user_context);
    void FinishTask(void *user_task, void *user_context);
    int WorkerCount();
} // namespace atmo::core::ecs::physics
