# ZeroUI - An immediate mode (~ish) game UI library

 This is an immediate mode UI library which fills a specific niche, in-game (i.e. not tool) UI primarily targeting gamepads.

Unlike most UI libraries, this one is targeted mainly towards programmers, not UI designers. As a programmer, I'm comfortable with programming, but I'm not particularly comfortable with all the abstractions and complexity that comes with most UI libraries. This library is intended for me and people like me. There's a very good chance this library is not for you, but that's fine, there are tons of other ones to fit other niches.

If gamepad support is not important to you, you are probably a billion times better off with [dear-imgui](https://github.com/ocornut/imgui). Just use that instead.

## Design goals
* Primarily gamepad as input, but compatible with mouse/touch
* User owns all "UI" state, easy to rollback
* Immediate-mode~ish, API's designed similarly to dear-imgui and such where it makes sense
* Easy to create custom rendering functions for widgets
* Easy to extend with custom widgets
* API/engine independent, output is vertices to stream to GPU
* KISS - Keep It Simple Stupid
* As always, no exceptions, no RTTI, no STL, no object-oriented programming. 

## Quick example

```cpp
zui::treeBegin("tree_list_id", f32x2(22.5f, 5.0f), 0.0f, 5.0f);

if (zui::treeCollapsableBegin("entry1_id", "Entry 1")) {
    zui::listBegin("entry1_list_id", 5.0f);

    if (zui::button("button1_id", "Button 1")) {

    }

    if (zui::button("button2_id", "Button 2")) {

    }

    zui::listEnd();
    zui::treeCollapsableEnd();
}

if (zui::treeCollapsableBegin("entry2_id", "Entry 2")) {
    zui::listBegin("entry2_list_id", 5.0f);

    if (zui::button("button3_id", "Button 3")) {

    }

    if (zui::button("button4_id", "Button 4")) {

    }

    zui::listEnd();
    zui::treeCollapsableEnd();
}

zui::treeEnd();
```

![](./docs/zeroui_example.gif)


## Current status

__Status: Early alpha__

ZeroUI is currently in an early-alpha (or prototype) state. I'm using it for the UI in the game I'm making and expanding it whenever I need to.

Feel free to try it out, but be aware that the API isn't stable and things might change. Documentation is incomplete, samples are missing, and overall you should be expecting to read a lot of source code to figure out how things fit together. Hopefully my design and code is clean enough that it makes sense, but we'll see how that goes.


## Dependencies and building

ZeroUI depends on [skipifzero core](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-Core) and [stb_truetype.h](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/stb). You can easily gather all the required headers from these libraries and dump them inside the same directory as the ZeroUI files and you are good to go.


## Documentation and samples

I try to keep most of the documentation inside the header files, most of it is in `ZeroUI.hpp`. Even though I have written quite a bit of documentation now I still feel it's sort of inadequate. Expect to read a lot of source code if you intend to use this.

Writing an engine integration shouldn't be particularly hard if you have ever integrated dear-imgui or similar before. But unfortunately I don't have any sample integration (mine is very specific to the game I'm making and written in ZeroG), and I don't really have the time or energy for making one right now. If you are interested in trying out ZeroUI feel free to contact me and I will help out with any questions you may have.
