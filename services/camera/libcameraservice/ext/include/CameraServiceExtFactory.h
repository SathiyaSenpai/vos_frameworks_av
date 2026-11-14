#pragma once

#include <binder/Parcel.h>

namespace android {

// Forward declaration – we will not define this class
class ICameraServiceExt;

class CameraServiceExtFactory {
public:
    // Returns the singleton instance of ExtFactory
    static void* getInstance();
    static int onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags);
    virtual ~CameraServiceExtFactory();

private:
    static void ensureLoaded();
    static void* sExtFactory;       // the singleton ExtFactory object
    static void* sExtObject;        // the real extension object (as void*)
    static int (*sOnTransactFunc)(void*, uint32_t, const Parcel&, Parcel*, uint32_t);
};

} // namespace android
