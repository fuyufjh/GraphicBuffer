# GraphicBuffer

Use GraphicBuffer class in Android native code in your project, without compiling with Android source code.

Inspired by [tcuAndroidInternals.cpp](https://android.googlesource.com/platform/external/deqp/+/master/framework/platform/android/tcuAndroidInternals.cpp)

# Examples

The usage is exactly the same with `android::GraphicBuffer`

Intialize:

```c++
int usage = GraphicBuffer::USAGE_HW_TEXTURE | GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_RARELY;
auto graphicBuf = std::make_unique<GraphicBuffer>(width, height, PIXEL_FORMAT_RGBA_8888, usage);
auto clientBuf = (EGLClientBuffer) graphicBuf->getNativeBuffer();
// ... more things
```

Get content image:

```c++
glFinish();
void *readPtr, *writePtr;
graphicBuf->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, &readPtr);
writePtr = image.data;
int stride = graphicBuf->getStride();
for (int row = 0; row < height; row++) {
    memcpy(writePtr, readPtr, width * 4);
    readPtr = (void *)(int(readPtr) + stride * 4);
    writePtr = (void *)(int(writePtr) + width * 4);
}
graphicBuf->unlock();
```
