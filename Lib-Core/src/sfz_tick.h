// Copyright 2019-2023 Peter Hillerström and skipifzero, all rights reserved.

#pragma once

#include <sfz.h>

// Ticks vs frames, fixed framerates vs dynamic framerates
// ------------------------------------------------------------------------------------------------

// This file is fairly simple, but its motivation is long and kinda complicated, so strap in.
//
// For various dumb reasons, the time between frames is usually dynamic and different every time.
// If we could guarantee that user has a 60hz display, doesn't use adaptive sync and never has any
// framedrops that would be ideal, because then we could asume 1/60 seconds has passed since the
// last frame and we would get perfect frame pacing. Unfortunately this is not the reality we live
// in.
//
// We can't update the game simulation with a variable time delta, because that has a tendency to
// make the simulation unstable and unpredictable. Not good. We need fixed timestep updates.
// Therefore we introduce the concept of a "Tick".
//
// Tick: A tick is a fixed-timestep update of the simulation. It is disconnected from framerate, we
//       can have multiple or no tick updates per frame update. Each frame we accumulate time that
//       has passed, and we use up as much time for tick updates as we can. Leftover time is saved
//       for next frame.
//
// However, this setup is non-ideal. It introduces a bunch of complexity wrt to handling the
// synchronization between ticks and frames. Examples:
//  * If our tick rate is lower than our frame rate we need to start interpolating objects to "make
//    up" frames. We also end up having to add at least one tick of input lag in order to support
//    this interpolation.
//  * Some logic, such as camera updates, really want to run per frame for maximum smoothness. This
//    is troublesome because then you end up separating state that should ideally be part of the
//    ("per-tick") game state into some weird "per-frame" game state.
//  * You end up having to separate input into "frame input" and "tick input", because tick logic
//    can't use normal frame input. If you use frame input for tick logic you end up dropping inputs
//    or reusing the same input multiple times for multiple ticks.
//
// Ideally, we would want exactly 1 tick update every frame, regardless of how much time has passed.
// So that's what we do. In order to accomplish this, we introduce the concept of "atomic tick" and
// "merged tick".
//
// Atomic tick: An atomic tick exactly matches what we previously referred to as a tick. It's the
//               smallest amount of time we can update the simulation with.
//
// Merged tick: A merged tick is an (integer) multiple of atomic ticks. The idea is that as long
//              as the timestep is a multiple of the atomic tick timestep, then it should probably
//              be fine to use the longer timestep for a bigger tick update.
//
// This is not perfect of course and probably a bit naive in some sense, but to make sure we don't
// shoot ourselves in foot completely we do 2 things:
// 1. We choose a pretty small atomic timestep (360hz), which should give us some wiggle-room later
//    one to ensure that the game doesn't feel "choppy".
// 2. We make sure that we always bundle the merged tick timestep with the integer amount of atomic
//    ticks that the merged tick consists of. This way sensitive game logic can choose to always
//    update using the atomic timestep and a for loop.
//
// Example of 2:
//
// // Using the merged timestep
// pos += velocity * merged_timestep;
//
// // Sensitive code, uses atomic timestep
// for (i32 i = 0; i < num_atomic_ticks; i++) {
//     pos += velocity * atomic_timestep;
// }
//
//
// In other words, we always have exactly 1 merged tick per frame. This merged tick consists of as
// many atomic ticks as we can fit in the time since last frame (+any unused leftover time). If we
// somehow end up rendering frames faster than our tick rate, we simply sleep a bit at the beginning
// of the frame before polling input. If we end up with a very bad framerate (less than ~15-20fps)
// we drop time and and instead slow down the simulation.
//
//
// A note regarding determinism:
// In order to be deterministic when replaying the simulation using stored input we must use the
// exact same merged ticks as was stored when recording. I.e., if you recorded with "20hz merged
// ticks" you can only replay using the same "20hz merged ticks". Or put yet another way, you can
// only replay at the exact same framerate you recorded at. This feels like a fair trade-off to
// make, but it certainly is worth noting regardless.

// Atomic tick
// ------------------------------------------------------------------------------------------------

// 360 is the best number, thus we have an atomic tick rate of 360hz.
//
// Merged tick rates for various multiples:
// 1:  360 / 1  = 360
// 2:  360 / 2  = 180
// 3:  360 / 3  = 120
// 4:  360 / 4  = 90
// 5:  360 / 5  = 72
// 6:  360 / 6  = 60
// 7:  360 / 7  = 51.43
// 8:  360 / 8  = 45
// 9:  360 / 9  = 40
// 10: 360 / 10 = 36
// 11: 360 / 11 = 32.73
// 12: 360 / 12 = 30
// 13: 360 / 13 = 27.69
// 14: 360 / 14 = 25.71
// 15: 360 / 15 = 24
// 16: 360 / 16 = 22.5
// 17: 360 / 17 = 21.18
// 18: 360 / 18 = 20
// 19: 360 / 19 = 18.95
// 20: 360 / 20 = 18
// 21: 360 / 21 = 17.14
// 22: 360 / 22 = 16.36
// 23: 360 / 23 = 15.65
// 24: 360 / 24 = 15
//
// In practice, we probably shouldn't allow a merged tick with less than 3 atomic ticks in it (i.e.
// 120hz). Supporting more than 120fps is a fool's errand, reserved for stupid e-sports titles.
//
// It also probably makes sense to not allow more than 24 atomic ticks per merged tick, as 15 fps
// can probably be considered the lowest bound of playable.

sfz_constant i32 SFZ_TICK_ATOMIC_REFRESH_RATE = 360;
sfz_constant f32 SFZ_TICK_ATOMIC_DELTA_SECS = 1.0f / f32(SFZ_TICK_ATOMIC_REFRESH_RATE);
sfz_constant f32 SFZ_TICK_ATOMIC_DELTA_MS = 1000.0f / f32(SFZ_TICK_ATOMIC_REFRESH_RATE);

// Merged tick
// ------------------------------------------------------------------------------------------------

sfz_constant i32 SFZ_TICK_MERGED_MIN_NUM_ATOMIC_TICKS = 3; // At least 120hz
sfz_constant i32 SFZ_TICK_MERGED_MAX_NUM_ATOMIC_TICKS = 24; // But no less than 15hz

sfz_struct(SfzTickMergedDelta) {

	// The number of "atomic ticks" this merged tick delta consists of.
	i32 num_atomic_ticks;

	// The merged tick time, equal to num_atomic_ticks * SFZ_TICK_ATOMIC_DELTA_SECS.
	f32 merged_tick_time_secs;
};
