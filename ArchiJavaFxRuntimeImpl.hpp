#pragma once
#include <condition_variable>
#include <deque>
#include <exception>
#include <jni.h>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include "ArchiJavaFxRuntime.hpp"
#include "ArchiWindowsHostModalGuard.hpp"
#include "ArchiWindowsJvmLoader.hpp"

namespace jnifx {
class ArchiJavaFxRuntime::Impl final {
public:
    explicit Impl(Config config);
    ~Impl();
    void startAsync();
    bool tryOpenWindow(std::string title, Arguments arguments, ResultCallback callback);
    bool tryOpenModalWindow(std::string title, Arguments arguments, ResultCallback callback);
    void shutdown() noexcept;
private:
    enum class State { Stopped, Starting, Ready, Stopping, Terminated, Failed };
    struct WindowCommand { RequestId requestId{}; WindowMode mode=WindowMode::Modeless; std::string title; Arguments arguments; };
    struct RequestState { WindowMode mode=WindowMode::Modeless; ResultCallback callback; };

    RequestId dispatchWindow(std::string, Arguments, WindowMode, ResultCallback);
    void executeWindow(JNIEnv*, WindowCommand);
    void cancelPendingCommandsLocked(const std::exception_ptr&) noexcept;
    void onWindowClosed(RequestId) noexcept;
    void onWindowResult(RequestId, JavaFxResult) noexcept;
    void onWindowFailed(RequestId, std::string) noexcept;
    void failAllRequests(const std::exception_ptr&) noexcept;
    jclass defineEmbeddedClasses(JNIEnv*);
    void registerNativeCallbacks(JNIEnv*);
    void unregisterNativeCallbacks(JNIEnv*) noexcept;
    void jvmMain();
    void createJvmAndWarmUpJavaFx();
    void shutdownOnJvmThread(JNIEnv*) noexcept;
    void cleanupJavaRefs(JNIEnv*) noexcept;
    void cleanupFailedJvm() noexcept;

    static void JNICALL nativeWindowClosed(JNIEnv*, jclass, jlong);
    static void JNICALL nativeWindowResult(JNIEnv*, jclass, jlong, jstring, jobjectArray);
    static void JNICALL nativeWindowFailed(JNIEnv*, jclass, jlong, jstring);

    Config config_;
    std::mutex lifecycleMutex_;
    mutable std::mutex mutex_;
    std::condition_variable readyCondition_;
    std::condition_variable commandCondition_;
    std::thread jvmThread_;
    State state_=State::Stopped;
    bool stopRequested_=false;
    bool modalBusy_=false;
    RequestId nextRequestId_=0;
    std::deque<WindowCommand> commands_;
    std::unordered_map<RequestId, RequestState> requests_;
    std::string lastError_;
    detail::ArchiWindowsHostModalGuard hostModalGuard_;
    detail::ArchiWindowsJvmLoader jvmLoader_;
    JavaVM* vm_=nullptr;
    jclass bridgeClass_=nullptr;
    jclass stringClass_=nullptr;
    jobject embeddedClassLoader_=nullptr;
    jmethodID initializeMethod_=nullptr;
    jmethodID openWindowMethod_=nullptr;
    jmethodID shutdownMethod_=nullptr;
};
} // namespace jnifx
