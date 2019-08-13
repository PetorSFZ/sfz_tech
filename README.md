# Phantasy Engine

Phantasy Engine is a barebones, cross-platform, DIY, game engine. DIY in the sense that you are expected to configure a lot of stuff specifically for your use case. It is mainly written to be the base for [skipifzero](skipifzero)'s games, but you are free to use it yourself if you are interested.

The engine is cross-platform, however it uses [ZeroG](https://github.com/PetorSFZ/ZeroG) for rendering. In other words, only platforms supported by ZeroG can be deployed on. At the time of writing ZeroG only has a D3D12 implementation, meaning Windows is the only possible target.

Before the switch to ZeroG, OpenGL was used. Then the engine could be compiled and deploy on the following platforms:

* Windows
* macOS
* iOS
* Web (via Emscripten)

If you are interested in trying out that version of the engine, check out the [LegacyRenderer](https://github.com/PetorSFZ/PhantasyEngine/tree/LegacyRenderer) branch.

## Usage/testing

If you want to try out Phantasy Engine for yourself, the best starting position is the [PhantasyTestbed](https://github.com/PetorSFZ/PhantasyTestbed). PhantasyTestbed is a small testbed application which loads and renders the classic Sponza testscene.

## License

Licensed under zlib, this means that you can basically use the code however you want as long as you give credit and don't claim you wrote it yourself. See LICENSE file for more info.

Libraries used by PhantasyEngine fall under various licenses, see their respective LICENSE files.

