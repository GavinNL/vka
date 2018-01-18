# VKA - Vulkan Assistant


VKA (Vulkan assistant) is a vulkan helper library to assist in working with
vulkan objects. It is very much a work in progress as I continue to teach myself
to use vulkan.

The intent of the library is to provide helper functions to generate objects
that are commonly used for rendering 3D graphics such as buffers, textures,
etc.  For example, setting up a texture may take many lines of code to
initialize. Using vka, we can create a texture by:

```C++
vka::texture2d * tex = context.new_texture2d("my_texture");
tex->set_size( 1024, 1024 );
tex->set_format(vk::Format::eR8G8B8A8Unorm);
tex->set_mipmap_levels(1);
tex->create();
tex->create_image_view(vk::ImageAspectFlagBits::eColor);
```

## License

Copyright 2018 GavinNL

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


## Dependencies

VKA uses the following libraries.
 * GLM - for linear algebra
 * STB - for image loading

If you already have GLM and STB installed on your system. Simply clone the
repo with the following command:

```bash
git clone http://github.com/gavinNL/vka.git
```

If you do not have GLM or STB, you can recursively download the submodules
using the ```--recursive comand```

```bash
git clone --recursive http://github.com/gavinNL/vka.git
```

## Build Instructions

```bash
git clone --recursive http://github.com/gavinNL/vka.git
cd vka
mkdir build
cd build
cmake ..
make
ln -s $PWD/../resources resources
```

## Examples

### Example_01 - Hello Rotating Textured Triangle

<img align="right" width="200"  src="docs/images/example_01.jpeg">

This example sets up a vulkan window, loads a texture and then draws a triangle
using the texture while animating it rotating. This example shows the simple
concept of the graphics pipeline.

---

### Example_02 - Depth Testing

<img align="right" width="200"  src="docs/images/example_02.jpeg">

This example demonstrates how to setup a rendering pipeline using depth
testing. Depth testing is an integral part of almost all rendering pipelines.

---

### Example_03 - Dynamic Uniform Buffers

<img align="right" width="200"  src="docs/images/example_03.jpeg">

This example demonstrates how to use dynamic uniform buffers to pass data to
each object drawn.

---
