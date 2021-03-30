# GraphicBuffer

Use GraphicBuffer class in Android native code in your project, without compiling with Android source code.

This repository is for APIs 23-27. API 23 is supported without additional tricks, APIs 24-25 need making your application a system application.

APIs 26 and 27 do not need code from this repository since a more convenient alternative is available: `HardwareBuffer`.

Moreover, this README provides an example of usage of the buffer to obtain a rendered texture image using simple and fast `memcpy()` calls, both for `GraphicBuffer` (API <= 23) and `HardwareBuffer` (API >= 26).

Inspired by [tcuAndroidInternals.cpp](https://android.googlesource.com/platform/external/deqp/+/master/framework/platform/android/tcuAndroidInternals.cpp)

# How to use

The usage is exactly the same with `android::GraphicBuffer` on API <= 25 or `HardwareBuffer` on API >= 26.
The example below shows a pseudo-code which renders something to a texture attached to a framebuffer and get the result using simple `memcpy()` calls.
Examples for both API >= 26 (HardwareBuffer) and API < 26 (GraphicBuffer) are provided.
If something doesn't work, it's worth checking if pointers are valid, if `eglGetError()` shows no issues, if there are any errors from Android system and also checking return codes with `glGetError()` if drawing issues occur.

An example for API <= 25 using this repository, GraphicBuffer:
```c++
// for EGL calls
#define EGL_EGLEXT_PROTOTYPES
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES/gl.h"
#define GL_GLEXT_PROTOTYPES
#include "GLES/glext.h"

// Use code from this repository. Note that define __ANDROID_API__ must be set properly for it to work
// Also add -lEGL at link stage
#include "GraphicBuffer.h" 

// bind FBO (create FBO my_handle first!)
glBindFramebuffer(GL_FRAMEBUFFER, my_handle);

// attach texture to FBO (create texture my_texture first!)
glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, my_texture, 0);

// usage for the GraphicBuffer
int usage = GraphicBuffer::USAGE_HW_RENDER | GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_NEVER;

// create GraphicBuffer
GraphicBuffer* graphicBuf = new GraphicBuffer(width, height, PIXEL_FORMAT_RGBA_8888, usage);

// get the native buffer
auto clientBuf = (EGLClientBuffer) graphicBuf->getNativeBuffer();

// obtaining the EGL display
EGLDisplay disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);

// specifying the image attributes
EGLint eglImageAttributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};

// creating an EGL image
EGLImageKHR imageEGL = eglCreateImageKHR(disp, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuf, eglImageAttributes);

// Doing some OpenGL rendering like glDrawArrays
// Shaders also work, need `#extension GL_OES_EGL_image_external : require`
// Now the result is inside the FBO my_handle

// binding the OUTPUT texture
glBindTexture(GL_TEXTURE_2D, my_texture);

// attaching an EGLImage to OUTPUT texture
glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, imageEGL);

// Obtaining the content image:

// pointer for reading and writing texture data
void *readPtr, *writePtr;

// locking the buffer
graphicBuf->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, &readPtr);

// setting the write pointer
writePtr = <set to a valid memory area, like malloc(_YOUR_SIZE_)>

// obtaining the stride (for me it was always = width)
int stride = graphicBuf->getStride();

// loop over texture rows
for (int row = 0; row < height; row++) {
    // copying, 4 = 4 channels RGBA because of the format above
    memcpy(writePtr, readPtr, width * 4);

    // adding stride * 4 to read pointer
    readPtr = (void *)(int(readPtr) + stride * 4);

    // adding width * 4 to write pointer
    writePtr = (void *)(int(writePtr) + width * 4);
}

// NOW data is in writePtr memory

// unlocking the buffer
graphicBuf->unlock();
```

Example for API >= 26. This repository is NOT needed, because there is an open alternative in NDK [1].
The example does exactly the same thing as the one above.
```c++
// for EGL calls
#define EGL_EGLEXT_PROTOTYPES
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES/gl.h"
#define GL_GLEXT_PROTOTYPES
#include "GLES/glext.h"

// for API >= 26
// Also add -lEGL -lnativewindow -lGLESv3 at link stage
#include "android/hardware_buffer.h"

// bind FBO (create FBO my_handle first!)
glBindFramebuffer(GL_FRAMEBUFFER, my_handle);

// attach texture to FBO (create texture my_texture first!)
glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, my_texture, 0);

// OUR parameters that we will set and give it to AHardwareBuffer
AHardwareBuffer_Desc usage;

// filling in the usage for HardwareBuffer
usage.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
usage.height = outputHeight;
usage.width = outputWidth;
usage.layers = 1;
usage.rfu0 = 0;
usage.rfu1 = 0;
usage.stride = 10;
usage.usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_NEVER
        | AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT;

// create GraphicBuffer
AHardwareBuffer* graphicBuf;
AHardwareBuffer_allocate(&usage, &graphicBuf); // it's worth to check the return code

// ACTUAL parameters of the AHardwareBuffer which it reports
AHardwareBuffer_Desc usage1;

// for stride, see below
AHardwareBuffer_describe(graphicBuf, &usage1);

// get the native buffer
EGLClientBuffer clientBuf = eglGetNativeClientBufferANDROID(graphicBuf);

// obtaining the EGL display
EGLDisplay disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);

// specifying the image attributes
EGLint eglImageAttributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};

// creating an EGL image
EGLImageKHR imageEGL = eglCreateImageKHR(disp, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuf, eglImageAttributes);
/**
 * @note this part should be earlies than any draw or framebuffer options.
 * @note refer to answer of @solidpixel at https://stackoverflow.com/questions/64447069/use-gleglimagetargettexture2does-to-replace-glreadpixels-on-android
 * @{
 */
// binding the OUTPUT texture
gl->glBindTexture(GL_TEXTURE_2D, my_texture);

// attaching an EGLImage to OUTPUT texture
glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, imageEGL);
/**
 * @}
 */
// Doing some OpenGL rendering like glDrawArrays
// Shaders also work, need `#extension GL_OES_EGL_image_external_essl3 : require`
// Now the result is inside the FBO my_handle

// Obtaining the content image:

// pointer for reading and writing texture data
void *readPtr, *writePtr;
/**
 * @note We must make sure all drawing options finished before read back.
 * @{
 */
 glFinish();
 /**
 * @}
 */
// locking the buffer
AHardwareBuffer_lock(graphicBuf, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr, (void**) &readPtr); // worth checking return code

// setting the write pointer
writePtr = <set to a valid memory area, like malloc(_YOUR_SIZE_)>

// obtaining the stride (for me it was always = width)
int stride = usage1.stride;

// loop over texture rows
for (int row = 0; row < height; row++) {
    // copying, 4 = 4 channels RGBA because of the format above
    memcpy(writePtr, readPtr, width * 4);

    // adding stride * 4 to read pointer
    readPtr = (void *)(int(readPtr) + stride * 4);

    // adding width * 4 to write pointer
    writePtr = (void *)(int(writePtr) + width * 4);
}

// NOW data is in writePtr memory

// unlocking the buffer
AHardwareBuffer_unlock(graphicBuf, nullptr); // worth checking return code
```

# How to access private libraries on API 24-25
On API 26, there is a public `HardwareBuffer` [1] option which replaces GraphicBuffer hacks. On API <= 23 the hack from the repo worked because the access to private libraries such as `libui.so` was allowed.

It's still allowed [2] in 24-25, however, `libui.so` also requires `gralloc.exynos5.so` (see full list of its dependencies [3]) which is **not allowed to use on API 24-25**. The app is killed when trying to dlopen `libui.so` (on `new GraphicBuffer()`).

It seems that there is a solution for API <= 23 and for API >= 26, but on 24 and 25 it seems that it's impossible to use any kind of GraphicBuffer-like access.

The solution for API 24-25, along with using code from this repository, is to make your application a system application.
It requires root privileges.
The process is described in https://stackoverflow.com/questions/24641604/qt-application-as-system-app-on-android for Qt-based apps.

# How to tweak API

The API 23 version in https://github.com/fuyufjh/GraphicBuffer/blob/fa346e1f6266a717758d32aee9c75c85da8a7263/GraphicBuffer.cpp uses the `_ZN7android13GraphicBufferC1Ejjij` constructor symbol, which was replaced by `_ZN7android13GraphicBufferC1EjjijNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE` in API 24-25 (a std::string argument added at the end). This repository works for the API 24-25 as well.

Since I'm not sure if any other APIs have different constructors, below you can find directions on how to tweak the code for your API.

1. Copy your file `/system/lib/libui.so` from your Android device to your PC. This is the file that contains symbol names for `GraphicBuffer`.
2. Using Android NDK's `nm` for your architecture, run:
```
$ /somewhere/android-ndk/find-it/arm-linux-androideabi-gcc-nm -C -D libui.so | grep GraphicBuffer | sort
```

It will produce output similar to this:

```
<other stuff>
0000de2c T android::GraphicBuffer::GraphicBuffer()
0000de2c T android::GraphicBuffer::GraphicBuffer()
0000dec8 T android::GraphicBuffer::GraphicBuffer(unsigned int, unsigned int, int, unsigned int, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >)
0000dec8 T android::GraphicBuffer::GraphicBuffer(unsigned int, unsigned int, int, unsigned int, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >)
0000dfd8 T android::GraphicBuffer::initSize(unsigned int, unsigned int, int, unsigned int, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >)
0000e074 T android::GraphicBuffer::GraphicBuffer(unsigned int, unsigned int, int, unsigned int, unsigned int, native_handle*, bool)
0000e074 T android::GraphicBuffer::GraphicBuffer(unsigned int, unsigned int, int, unsigned int, unsigned int, native_handle*, bool)
0000e120 T android::GraphicBuffer::GraphicBuffer(ANativeWindowBuffer*, bool)
0000e120 T android::GraphicBuffer::GraphicBuffer(ANativeWindowBuffer*, bool)
<other stuff>
```

Find a constructor that is suitable for you. Try googling for source code of `GraphicBuffer.cpp` for your API, for example: https://android.googlesource.com/platform/frameworks/native/+/jb-dev/libs/ui/GraphicBuffer.cpp. Once you have identified the constructor signature you would like to use, find the name of it's symbol in

```
# same as above but without -C
$ /somewhere/android-ndk/find-it/arm-linux-androideabi-gcc-nm -D libui.so | grep GraphicBuffer | sort
```

Copy the name of the symbol, for example: `_ZN7android13GraphicBufferC1EjjijNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE`. It corresponds 1-to-1 to the human-readable signature. For my API 24-25 the difference from API 23 code was that there was an `std::string` argument added.

For lower APIs (unclear which ones) the solution uses `_ZN7android13GraphicBufferC1Ejjij` which is absent in API 24-25.

3. Having identified your constructor, change the code in `GraphicBuffer.cpp` appropriately. For API 24-25 a `std::string` argument was added, it has to be passed by reference and the variable with the string should remain after function call ends (I just made it `static`).
4. Debug the code using `print` statements showing error codes

[1] https://developer.android.com/ndk/guides/stable_apis https://developer.android.com/reference/android/hardware/HardwareBuffer

[2] https://developer.android.com/about/versions/nougat/android-7.0-changes

[3] `libbacktrace.so libbase.so libbinder.so libc.so libc++.so libcutils.so libdl.so gralloc.exynos5.so libhardware.so libion.so liblog.so liblzma.so libm.so libsync.so libui.so libunwind.so libutils.so`, obtained by a simple recursive jupyter notebook using `!{readelf_bin} -d {file} | grep NEEDED`
