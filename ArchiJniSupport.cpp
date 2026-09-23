#include "ArchiJniSupport.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace jnifx::detail {
    namespace {

        std::vector<jchar> decodeUtf8(const std::string& text) {
            std::vector<jchar> result;
            result.reserve(text.size());

            for (std::size_t i = 0; i < text.size();) {
                const auto first = static_cast<std::uint8_t>(text[i]);
                std::uint32_t codePoint = 0;
                std::size_t trailing = 0;

                if (first <= 0x7F) {
                    codePoint = first;
                } else if (first >= 0xC2 && first <= 0xDF) {
                    codePoint = first & 0x1F;
                    trailing = 1;
                } else if (first >= 0xE0 && first <= 0xEF) {
                    codePoint = first & 0x0F;
                    trailing = 2;
                } else if (first >= 0xF0 && first <= 0xF4) {
                    codePoint = first & 0x07;
                    trailing = 3;
                } else {
                    throw std::invalid_argument("UTF-8 text invalid");
                }

                if (i + trailing >= text.size()) {
                    throw std::invalid_argument("Texte UTF-8 tronque");
                }
                for (std::size_t offset = 1; offset <= trailing; ++offset) {
                    const auto next = static_cast<std::uint8_t>(text[i + offset]);
                    if ((next & 0xC0) != 0x80) {
                        throw std::invalid_argument("UTF-8 text invalid");
                    }
                    codePoint = (codePoint << 6) | (next & 0x3F);
                }

                const bool overlong = (trailing == 1 && codePoint < 0x80) || (trailing == 2 && codePoint < 0x800) ||
                    (trailing == 3 && codePoint < 0x10000);
                if (overlong || codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF)) {
                    throw std::invalid_argument("UTF-8 text invalid");
                }

                if (codePoint <= 0xFFFF) {
                    result.push_back(static_cast<jchar>(codePoint));
                } else {
                    codePoint -= 0x10000;
                    result.push_back(static_cast<jchar>(0xD800 + (codePoint >> 10)));
                    result.push_back(static_cast<jchar>(0xDC00 + (codePoint & 0x3FF)));
                }
                i += trailing + 1;
            }
            return result;
        }

    }    // namespace

    void throwIfJavaException(JNIEnv* env, const char* operation) {
        if (!env->ExceptionCheck()) {
            return;
        }

        jthrowable exception = env->ExceptionOccurred();
        env->ExceptionClear();

        std::string detail;
        jclass exceptionClass = exception == nullptr ? nullptr : env->GetObjectClass(exception);
        jmethodID toString =
            exceptionClass == nullptr ? nullptr : env->GetMethodID(exceptionClass, "toString", "()Ljava/lang/String;");

        if (!env->ExceptionCheck() && toString != nullptr) {
            jstring message = static_cast<jstring>(env->CallObjectMethod(exception, toString));
            if (!env->ExceptionCheck() && message != nullptr) {
                const char* text = env->GetStringUTFChars(message, nullptr);
                if (text != nullptr) {
                    detail = text;
                    env->ReleaseStringUTFChars(message, text);
                }
                env->DeleteLocalRef(message);
            }
        }
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
        if (exceptionClass != nullptr) {
            env->DeleteLocalRef(exceptionClass);
        }
        if (exception != nullptr) {
            env->DeleteLocalRef(exception);
        }

        std::string message = std::string("Exception Java pendant : ") + operation;
        if (!detail.empty()) {
            message += " (" + detail + ')';
        }
        throw std::runtime_error(message);
    }

    jstring newJavaString(JNIEnv* env, const std::string& utf8) {
        const std::vector<jchar> utf16 = decodeUtf8(utf8);
        if (utf16.size() > static_cast<std::size_t>(std::numeric_limits<jsize>::max())) {
            throw std::length_error("Chaine trop longue pour JNI");
        }

        jstring value = env->NewString(utf16.empty() ? nullptr : utf16.data(), static_cast<jsize>(utf16.size()));
        if (value == nullptr) {
            throwIfJavaException(env, "allocation d'une chaine");
            throw std::runtime_error("Impossible d'allouer une chaine Java");
        }
        return value;
    }

    std::string exceptionMessage(const std::exception_ptr& error) {
        if (!error) {
            return {};
        }
        try {
            std::rethrow_exception(error);
        } catch (const std::exception& exception) {
            return exception.what();
        } catch (...) {
            return "Erreur native inconnue";
        }
    }

    ThreadAttachment::ThreadAttachment(JavaVM* vm) : vm_(vm) {
        const jint status = vm_->GetEnv(reinterpret_cast<void**>(&env_), JNI_VERSION_1_8);
        if (status == JNI_EDETACHED) {
            if (vm_->AttachCurrentThread(reinterpret_cast<void**>(&env_), nullptr) != JNI_OK) {
                throw std::runtime_error("Impossible d'attacher le thread C++ a la JVM");
            }
            detach_ = true;
        } else if (status != JNI_OK) {
            throw std::runtime_error("Version JNI incompatible");
        }
    }

    ThreadAttachment::~ThreadAttachment() {
        if (detach_) {
            vm_->DetachCurrentThread();
        }
    }

    JNIEnv* ThreadAttachment::env() const noexcept {
        return env_;
    }

    LocalFrame::LocalFrame(JNIEnv* env, jint capacity) : env_(env) {
        if (env_->PushLocalFrame(capacity) != JNI_OK) {
            throwIfJavaException(env_, "reservation des references JNI");
            throw std::runtime_error("Impossible de reserver les references JNI locales");
        }
    }

    LocalFrame::~LocalFrame() {
        env_->PopLocalFrame(nullptr);
    }

}    // namespace jnifx::detail
