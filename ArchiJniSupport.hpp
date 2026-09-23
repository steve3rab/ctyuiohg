#pragma once

#include <exception>
#include <jni.h>
#include <string>

namespace jnifx::detail {

    void throwIfJavaException(JNIEnv* env, const char* operation);
    jstring newJavaString(JNIEnv* env, const std::string& utf8);
    std::string exceptionMessage(const std::exception_ptr& error);

    class ThreadAttachment final {
      public:
        explicit ThreadAttachment(JavaVM* vm);
        ~ThreadAttachment();

        ThreadAttachment(const ThreadAttachment&) = delete;
        ThreadAttachment& operator=(const ThreadAttachment&) = delete;

        JNIEnv* env() const noexcept;

      private:
        JavaVM* vm_;
        JNIEnv* env_ = nullptr;
        bool detach_ = false;
    };

    class LocalFrame final {
      public:
        explicit LocalFrame(JNIEnv* env, jint capacity);
        ~LocalFrame();

        LocalFrame(const LocalFrame&) = delete;
        LocalFrame& operator=(const LocalFrame&) = delete;

      private:
        JNIEnv* env_;
    };

}    // namespace jnifx::detail
