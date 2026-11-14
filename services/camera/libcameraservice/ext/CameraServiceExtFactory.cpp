#include "CameraServiceExtFactory.h"
#include <dlfcn.h>
#include <log/log.h>

namespace android {

void* CameraServiceExtFactory::sExtFactory = nullptr;
void* CameraServiceExtFactory::sExtObject = nullptr;
int (*CameraServiceExtFactory::sOnTransactFunc)(void*, uint32_t, const Parcel&, Parcel*, uint32_t) = nullptr;

void CameraServiceExtFactory::ensureLoaded() {
    if (sExtFactory != nullptr) return;

    const char* libPath = "system_ext/lib64/libcsextimpl.so";
    void* handle = dlopen(libPath, RTLD_NOW);
    if (handle == nullptr) {
        ALOGE("CameraServiceExtFactory: dlopen failed: %s", dlerror());
        return;
    }
    ALOGI("CameraServiceExtFactory: dlopen succeeded, handle=%p", handle);

    typedef void* (*GetFactoryFunc)();
    GetFactoryFunc getExtFactoryImpl = (GetFactoryFunc)dlsym(handle, "getExtFactoryImpl");
    if (getExtFactoryImpl == nullptr) {
        ALOGE("CameraServiceExtFactory: dlsym getExtFactoryImpl failed: %s", dlerror());
        dlclose(handle);
        return;
    }
    ALOGI("CameraServiceExtFactory: getExtFactoryImpl at %p", getExtFactoryImpl);

    sExtFactory = getExtFactoryImpl();
    if (sExtFactory == nullptr) {
        ALOGE("CameraServiceExtFactory: getExtFactoryImpl returned null");
        dlclose(handle);
        return;
    }
    ALOGI("CameraServiceExtFactory: ext factory object at %p", sExtFactory);

    // Resolve onTransact (for direct call via symbol)
    sOnTransactFunc = (int (*)(void*, uint32_t, const Parcel&, Parcel*, uint32_t))
        dlsym(handle, "_ZN7android20CameraServiceExtImpl10onTransactEjRKNS_6ParcelEPS1_j");
    if (sOnTransactFunc == nullptr) {
        ALOGE("CameraServiceExtFactory: dlsym onTransact failed: %s", dlerror());
    } else {
        ALOGI("CameraServiceExtFactory: onTransact found at %p", sOnTransactFunc);
    }
}

void* CameraServiceExtFactory::getInstance() {
    ensureLoaded();
    return sExtFactory;
}

int CameraServiceExtFactory::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
    ensureLoaded();
    if (sExtObject == nullptr) {
        if (sExtFactory == nullptr) {
            ALOGE("CameraServiceExtFactory::onTransact: extension factory not loaded");
            return -1;
        }
        // ExtFactory's first vtable entry is getCameraServiceExt(ExtFactory* this)
        void** vtable = *(void***)sExtFactory;
        typedef void* (*GetCameraServiceExtFunc)(void*);
        GetCameraServiceExtFunc getCameraServiceExt = (GetCameraServiceExtFunc)vtable[0];
        sExtObject = getCameraServiceExt(sExtFactory);
        if (sExtObject == nullptr) {
            ALOGE("CameraServiceExtFactory: getCameraServiceExt returned null");
            return -1;
        }
        ALOGI("CameraServiceExtFactory: real extension object at %p", sExtObject);
    }

    if (sOnTransactFunc == nullptr) {
        ALOGE("CameraServiceExtFactory::onTransact: no function pointer");
        return -1;
    }
    return sOnTransactFunc(sExtObject, code, data, reply, flags);
}

CameraServiceExtFactory::~CameraServiceExtFactory() {
    // No cleanup needed – the extension library manages its own singleton.
    ALOGV("CameraServiceExtFactory destructor (stub)");
}

} // namespace android
