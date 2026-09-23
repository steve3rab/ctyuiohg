#pragma once

#ifndef _WIN32
#error JavaFxRuntime supports Windows 11 and later only.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <jni.h>
#include <string>
#include <windows.h>

namespace jnifx::detail {

    std::string expandClassPath(const std::string& classPath);

    class ArchiWindowsJvmLoader final {
      public:
        ArchiWindowsJvmLoader() = default;
        ~ArchiWindowsJvmLoader();

        ArchiWindowsJvmLoader(const ArchiWindowsJvmLoader&) = delete;
        ArchiWindowsJvmLoader& operator=(const ArchiWindowsJvmLoader&) = delete;

        void load(const std::string& configuredJvmPath, const std::string& jreEnvironmentVariable);
        void unload() noexcept;

        jint getCreatedJavaVms(JavaVM** buffer, jsize bufferLength, jsize* count) const;

        jint createJavaVm(JavaVM** vm, JNIEnv** env, JavaVMInitArgs* arguments) const;

      private:
        using CreateJavaVmFunction = jint(JNICALL*)(JavaVM**, void**, void*);
        using GetCreatedJavaVmsFunction = jint(JNICALL*)(JavaVM**, jsize, jsize*);

        HMODULE module_ = nullptr;
        DLL_DIRECTORY_COOKIE serverDirectoryCookie_ = nullptr;
        DLL_DIRECTORY_COOKIE binDirectoryCookie_ = nullptr;
        CreateJavaVmFunction createJavaVm_ = nullptr;
        GetCreatedJavaVmsFunction getCreatedJavaVms_ = nullptr;
    };

}    // namespace jnifx::detail
