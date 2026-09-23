#pragma once

#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <jni.h>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ArchiJavaFxRuntime.hpp"
#include "ArchiWindowsHostModalGuard.hpp"
#include "ArchiWindowsJvmLoader.hpp"

namespace jnifx {

class ArchiJavaFxRuntime::Impl final {
  public:
    explicit Impl(Config config);
    ~Impl();

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    void startAsync();
    bool tryOpenWindow(std::string title, Arguments arguments);
    bool tryOpenModalWindow(std::string title, Arguments arguments);
    ArchiJavaFxRuntime::State state() const noexcept;
    bool isReady() const noexcept;
    std::string lastError() const;
    void setResultCallback(ResultCallback callback);
    void shutdown() noexcept;

  private:
    struct WindowCommand final {
        RequestId requestId = 0;
        WindowMode mode = WindowMode::Modeless;
        std::string title;
        Arguments arguments;
    };

    struct RequestState final {
        WindowMode mode = WindowMode::Modeless;
    };

    RequestId dispatchWindow(std::string title, Arguments arguments, WindowMode mode);
    void executeWindow(JNIEnv* env, WindowCommand command);
    void onWindowClosed(RequestId requestId) noexcept;
    void onWindowResult(RequestId requestId, int status, std::vector<std::string> values) noexcept;
    void onWindowFailed(RequestId requestId, std::string message) noexcept;
    void failAllRequests(const std::exception_ptr& error) noexcept;
    void cancelPendingCommandsLocked(const std::exception_ptr& error) noexcept;

    jclass defineEmbeddedClasses(JNIEnv* env);
    void registerNativeCallbacks(JNIEnv* env);
    void unregisterNativeCallbacks(JNIEnv* env) noexcept;
    void jvmMain();
    void createJvmAndWarmUpJavaFx();
    void shutdownOnJvmThread(JNIEnv* env) noexcept;
    void cleanupJavaRefs(JNIEnv* env) noexcept;
    void cleanupFailedJvm() noexcept;

    static void JNICALL nativeWindowClosed(JNIEnv* env, jclass clazz, jlong requestId);
    static void JNICALL nativeWindowResult(
        JNIEnv* env, jclass clazz, jlong requestId, jint status, jobjectArray values);
    static void JNICALL nativeWindowFailed(JNIEnv* env, jclass clazz, jlong requestId, jstring message);

    Config config_;

    mutable std::mutex mutex_;
    std::mutex lifecycleMutex_;
    std::condition_variable readyCondition_;
    std::condition_variable commandCondition_;

    ArchiJavaFxRuntime::State state_ = ArchiJavaFxRuntime::State::Stopped;
    bool stopRequested_ = false;
    bool modalBusy_ = false;
    std::string lastError_;

    std::deque<WindowCommand> commands_;
    std::unordered_map<RequestId, RequestState> requests_;
    RequestId nextRequestId_ = 0;

    ResultCallback resultCallback_;

    std::thread jvmThread_;
    JavaVM* vm_ = nullptr;
    jclass bridgeClass_ = nullptr;
    jclass stringClass_ = nullptr;
    jobject embeddedClassLoader_ = nullptr;
    jmethodID initializeMethod_ = nullptr;
    jmethodID openWindowMethod_ = nullptr;
    jmethodID shutdownMethod_ = nullptr;

    detail::ArchiWindowsJvmLoader jvmLoader_;
    detail::ArchiWindowsHostModalGuard hostModalGuard_;
};

} // namespace jnifx
