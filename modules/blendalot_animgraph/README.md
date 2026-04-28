# Blendalot - A Magical Animation System for Godot

Blendalot is an experimental animation system for Godot that is currently in development. One of it's core features is a very flexible animation syncing mechanism that allows smooth transitions between related motions (e.g. walking, running, limping , ...). This is done by using SyncTracks as described by Bobby Anguelov here: https://www.youtube.com/watch?v=Jkv0pbp0ckQ&t=7998s.

## Status

The project is still very much work-in-progress and consider everything to be subject to change.

Current features:

* Animation graph evaluation with syncing support as a core principle.
* Hierarchical blend tree evaluation (blend trees containing state machines and vice versa)

Implemented nodes:

* Animation sampler
* Blend tree
  * Blend2
  * TimeScale
* State machine
  * Transitions (with optional syncing)

## License

Blendalot is distributed under the [MIT license](LICENSE).

