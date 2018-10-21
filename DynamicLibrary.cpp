#include "DynamicLibrary.h"

#include <dlfcn.h>
#include <android/log.h>

DynamicLibrary::DynamicLibrary(const char *fileName)
{
    libHandle = dlopen(fileName, RTLD_LAZY);
    if (!libHandle) throw OpenLibFailedException();
}

DynamicLibrary::~DynamicLibrary()
{
    if (libHandle) dlclose(libHandle);
}

void *DynamicLibrary::getFunctionPtr(const char *name) const
{
    auto ret = (void *) dlsym(libHandle, name);
    if (ret == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, "GraphicBuffer", "Failed to get function %s", name);
    }
    return ret;
}

