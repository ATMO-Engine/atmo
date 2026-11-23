---
id: signals-and-events
title: Signals and events
order: 18
description: For engine contributors. Notify other code with entity signals, global events and the signal queue.
---

# Signals and events

This page is for contributors to the engine. Neither signals nor events are available to scripts.

|               | Signals                     | Events                                           |
| ------------- | --------------------------- | ------------------------------------------------ |
| Belong to     | An entity                   | The whole engine                                 |
| Identified by | A name, such as `"Pressed"` | A C++ type                                       |
| Use for       | What happens to one entity  | What concerns code that does not know the source |
| End           | With the entity             | When the listener is removed                     |

## Signals {#signals}

### Create and emit {#create-signal}

Create signals in `initialize`, with the types of their arguments:

```cpp
createSignal<int>("Collected");
```

and emit them:

```cpp
getSignal<int>("Collected").emit(coin.value);
```

`getSignal` must be given the same types as `createSignal`. A wrong type or an unknown name is only detected by an
`assert`.

### Connect {#connect}

```cpp
coin.getSignal<int>("Collected").connect([handle = player.getHandle()](int value) {
    if (!handle.is_alive())
        return;
    // ...
});
```

- Capture handles by value and check `is_alive()`: the other entity can be destroyed before the signal is emitted.
- Callbacks are called in the order they were connected.
- A single callback cannot be disconnected. All of them go with the entity.
- Do not capture `this` in an entity method: entity objects are temporary. Capture `p_handle` by value instead.

When a signal is emitted while the world is read-only, as in a system, its callbacks are called at the end of the frame
instead of immediately.

### Signals of entities {#entity-signals}

| Signal          | Arguments                    | Emitted by                             |
| --------------- | ---------------------------- | -------------------------------------- |
| `child_added`   | `Entity`                     | The parent, in `setParent`             |
| `child_removed` | `Entity`                     | The parent, in `destroy`               |
| `renamed`       | `std::string`, `std::string` | The entity, with the old and new names |

### Signals of UI entities {#ui-signals}

| Entity           | Signal                                 | Arguments      | When                                                      |
| ---------------- | -------------------------------------- | -------------- | --------------------------------------------------------- |
| `UIButton`       | `MouseEntered`, `MouseExited`          |                | The mouse enters or leaves                                |
| `UIButton`       | `Pressed`, `Released`                  |                | The mouse button goes down or up. Not for toggle buttons. |
| `UIButton`       | `Toggle`                               | `bool`         | The state of a toggle button changes                      |
| `UISlider`       | `FloatValueChanged`, `IntValueChanged` | `float`, `int` | The value changes, depending on the type                  |
| `UIInput`        | `TextChanged`                          | `std::string`  | After each edit                                           |
| `UIInput`        | `Submitted`                            | `std::string`  | Enter is pressed, in a single-line field                  |
| `UIInput`        | `FocusChanged`                         | `bool`         | The field gains or loses the focus                        |
| `UITextInput`    | `StringValueChanged`                   | `std::string`  | An edited value is committed                              |
| `UINumberInput`  | `FloatValueChanged`, `IntValueChanged` | `float`, `int` | An edited value is committed                              |
| `UiDropdownMenu` | `Opened`, `Closed`                     |                | The menu opens or closes                                  |

When one button of a group is pressed, every button of the group emits `Toggle`.

## Events {#events}

### Declare an event {#declare-event}

`score_event.hpp`:

```cpp
#pragma once

#include "core/event/event_registry.hpp"
#include "core/event/events/event.hpp"

namespace atmo::core::event::events
{
    class ScoreEvent : public EventRegistry::Registrable<ScoreEvent, Event>
    {
    public:
        using EventRegistry::Registrable<ScoreEvent, Event>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "ScoreEvent";
        }

        int points = 0;
    };
} // namespace atmo::core::event::events
```

`score_event.cpp`:

```cpp
#include "score_event.hpp"

ATMO_REGISTER_EVENT(atmo::core::event::events::ScoreEvent);
```

### Listen and dispatch {#dispatch}

```cpp
using event::EventRegistry;
using event::events::ScoreEvent;

const std::uint32_t listener = EventRegistry::SetCallBack<ScoreEvent>([](ScoreEvent *event) {
    if (event->points >= 100)
        event->consume();
});

auto event = std::make_shared<ScoreEvent>();
event->points = 10;
EventRegistry::Dispatch(event);

EventRegistry::RemoveCallBack<ScoreEvent>(listener);
```

- `Dispatch` calls the listeners immediately, in the order they were added, until one consumes the event.
- Listeners only receive their exact type: a listener of `UIEvent` does not receive a `ButtonGroupToggleEvent`.
- Keep the number returned by `SetCallBack` and remove the listener when its owner is destroyed. For an entity, do it in
  an observer of `flecs::OnRemove`, as `UIButton` does.
- Do not add or remove listeners of a type while an event of that type is dispatched: the list is being read.

### Events of the engine {#engine-events}

| Event                                      | Dispatched                                      |
| ------------------------------------------ | ----------------------------------------------- |
| `ProgressTickEvent`                        | At the end of each frame, with `delta_time`     |
| `PhysicsProgressTickEvent`                 | After each physics step, with its `delta_time`  |
| `InputEvent`                               | For each input event of SDL, as `sdl_event`     |
| `ButtonGroupToggleEvent`                   | When a button of a group is pressed             |
| `DropdownGroupEvent`                       | When a dropdown of a group is hovered or closed |
| `FileSelectedEvent`, `ReloadExplorerEvent` | By the file explorer and the file watcher       |

## The signal queue {#queue}

Flecs does not allow entities to be created or destroyed while systems run. `SignalQueue::Enqueue` delays a function to
the end of the frame, when the world can be changed:

```cpp
SignalQueue::Enqueue([handle]() {
    if (handle.is_alive())
        ecs::entities::Entity(handle).destroy();
});
```

- It can be called from any thread. The functions run on the main thread.
- Functions added while the queue is emptied run in the same frame.
- Check that entities are still alive: another function may have destroyed them.

In each frame, the order is: the systems of the world, the signal queue, `ProgressTickEvent`, then the reset of the
"just pressed" inputs.
