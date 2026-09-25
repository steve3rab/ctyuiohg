#include "ArchiJavaFxRuntimeImpl.hpp"

#include <algorithm>
#include <atomic>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ArchiJniSupport.hpp"

namespace jnifx {
    namespace {

        std::mutex gJvmCreationMutex;
        std::atomic<void*> gActiveRuntime{ nullptr };

        std::exception_ptr makeError(const char* message) {
            return std::make_exception_ptr(std::runtime_error(message));
        }

        std::string javaStringToUtf8NoThrow(JNIEnv* env, jstring value) noexcept {
            if (env == nullptr || value == nullptr) {
                return {};
            }

            try {
                const jsize length = env->GetStringLength(value);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    return {};
                }

                std::vector<jchar> chars(static_cast<std::size_t>(length));
                if (length != 0) {
                    env->GetStringRegion(value, 0, length, chars.data());
                    if (env->ExceptionCheck()) {
                        env->ExceptionClear();
                        return {};
                    }
                }

                std::string result;
                result.reserve(static_cast<std::size_t>(length));

                for (std::size_t i = 0; i < chars.size(); ++i) {
                    std::uint32_t codePoint = chars[i];

                    if (codePoint >= 0xD800 && codePoint <= 0xDBFF) {
                        if (i + 1 < chars.size()) {
                            const std::uint32_t low = chars[i + 1];
                            if (low >= 0xDC00 && low <= 0xDFFF) {
                                codePoint = 0x10000u
                                    + ((codePoint - 0xD800u) << 10u)
                                    + (low - 0xDC00u);
                                ++i;
                            } else {
                                codePoint = 0xFFFDu;
                            }
                        } else {
                            codePoint = 0xFFFDu;
                        }
                    } else if (codePoint >= 0xDC00 && codePoint <= 0xDFFF) {
                        codePoint = 0xFFFDu;
                    }

                    if (codePoint <= 0x7Fu) {
                        result.push_back(static_cast<char>(codePoint));
                    } else if (codePoint <= 0x7FFu) {
                        result.push_back(static_cast<char>(0xC0u | (codePoint >> 6u)));
                        result.push_back(static_cast<char>(0x80u | (codePoint & 0x3Fu)));
                    } else if (codePoint <= 0xFFFFu) {
                        result.push_back(static_cast<char>(0xE0u | (codePoint >> 12u)));
                        result.push_back(static_cast<char>(0x80u | ((codePoint >> 6u) & 0x3Fu)));
                        result.push_back(static_cast<char>(0x80u | (codePoint & 0x3Fu)));
                    } else {
                        result.push_back(static_cast<char>(0xF0u | (codePoint >> 18u)));
                        result.push_back(static_cast<char>(0x80u | ((codePoint >> 12u) & 0x3Fu)));
                        result.push_back(static_cast<char>(0x80u | ((codePoint >> 6u) & 0x3Fu)));
                        result.push_back(static_cast<char>(0x80u | (codePoint & 0x3Fu)));
                    }
                }

                return result;
            } catch (...) {
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                }
                return {};
            }
        }

        std::vector<std::string> javaStringArrayToUtf8NoThrow(
            JNIEnv* env, jobjectArray values) noexcept {
            std::vector<std::string> result;
            if (env == nullptr || values == nullptr) {
                return result;
            }

            try {
                const jsize length = env->GetArrayLength(values);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    return {};
                }

                result.reserve(static_cast<std::size_t>(length));
                for (jsize i = 0; i < length; ++i) {
                    jstring value = static_cast<jstring>(env->GetObjectArrayElement(values, i));
                    if (env->ExceptionCheck()) {
                        env->ExceptionClear();
                        return {};
                    }

                    result.emplace_back(javaStringToUtf8NoThrow(env, value));
                    if (value != nullptr) {
                        env->DeleteLocalRef(value);
                    }
                }
            } catch (...) {
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                }
                return {};
            }

            return result;
        }

    }    // namespace

    ArchiJavaFxRuntime::Impl::Impl(Config config) : config_(std::move(config)) {
        if (config_.classPath.empty()) {
            throw std::invalid_argument("ArchiJavaFxRuntime::Config.classPath is mandatory");
        }
        if (config_.bridgeClass.empty()) {
            throw std::invalid_argument("ArchiJavaFxRuntime::Config.bridgeClass is mandatory");
        }
        if (config_.modalBlockTimeout <= std::chrono::milliseconds::zero()) {
            throw std::invalid_argument("ArchiJavaFxRuntime::Config.modalBlockTimeout must be positive");
        }
        if (config_.jvmPath.empty() && config_.jreEnvironmentVariable.empty()) {
            throw std::invalid_argument("Config.jreEnvironmentVariable is mandatory when Config.jvmPath is empty");
        }
        std::replace(config_.bridgeClass.begin(), config_.bridgeClass.end(), '.', '/');

        std::unordered_set<std::string> embeddedNames;
        for (auto& embedded : config_.embeddedClasses) {
            std::replace(embedded.binaryName.begin(), embedded.binaryName.end(), '.', '/');
            if (embedded.binaryName.empty()) {
                throw std::invalid_argument("Embedded class name is empty");
            }
            if (!embeddedNames.insert(embedded.binaryName).second) {
                throw std::invalid_argument("Embedded class name is redundant: " + embedded.binaryName);
            }
            if (embedded.bytecode.size() < 4 || embedded.bytecode[0] != 0xCA || embedded.bytecode[1] != 0xFE ||
                embedded.bytecode[2] != 0xBA || embedded.bytecode[3] != 0xBE) {
                throw std::invalid_argument("Bytecode .class invalid: " + embedded.binaryName);
            }
            if (embedded.bytecode.size() > static_cast<std::size_t>(std::numeric_limits<jsize>::max())) {
                throw std::length_error("Embedded class is too large: " + embedded.binaryName);
            }
        }
    }

    ArchiJavaFxRuntime::Impl::~Impl() {
        try {
            shutdown();
        } catch (...) {
            // Destructors must never escape.
        }
    }

    ArchiJavaFxRuntime::State ArchiJavaFxRuntime::Impl::state() const noexcept {
        std::lock_guard lock(mutex_);
        return state_;
    }

    bool ArchiJavaFxRuntime::Impl::isReady() const noexcept {
        std::lock_guard lock(mutex_);
        return state_ == ArchiJavaFxRuntime::State::Ready;
    }

    std::string ArchiJavaFxRuntime::Impl::lastError() const {
        std::lock_guard lock(mutex_);
        return lastError_;
    }

    void ArchiJavaFxRuntime::Impl::startAsync() {
        std::lock_guard lifecycleLock(lifecycleMutex_);
        std::lock_guard lock(mutex_);

        switch (state_) {
        case ArchiJavaFxRuntime::State::Stopped:
            break;
        case ArchiJavaFxRuntime::State::Starting:
        case ArchiJavaFxRuntime::State::Ready:
            return;
        case ArchiJavaFxRuntime::State::Stopping:
        case ArchiJavaFxRuntime::State::Terminated:
        case ArchiJavaFxRuntime::State::Failed:
            // This runtime is single-lifecycle. The service creates a fresh
            // runtime if the plugin is initialized again. Never restart a JVM
            // from a button callback.
            return;
        }

        state_ = ArchiJavaFxRuntime::State::Starting;
        stopRequested_ = false;
        modalBusy_ = false;
        lastError_.clear();
        commands_.clear();
        requests_.clear();

        try {
            jvmThread_ = std::thread([this] { jvmMain(); });
        } catch (...) {
            const std::exception_ptr error = std::current_exception();
            lastError_ = detail::exceptionMessage(error);
            state_ = ArchiJavaFxRuntime::State::Failed;
            readyCondition_.notify_all();
            throw;
        }
    }

    ArchiJavaFxRuntime::RequestId ArchiJavaFxRuntime::Impl::tryOpenWindow(std::string title, Arguments arguments) {
        try {
            return dispatchWindow(std::move(title), std::move(arguments), WindowMode::Modeless);
        } catch (...) {
            return 0;
        }
    }

    ArchiJavaFxRuntime::RequestId ArchiJavaFxRuntime::Impl::tryOpenModalWindow(std::string title, Arguments arguments) {
        try {
            return dispatchWindow(std::move(title), std::move(arguments), WindowMode::Modal);
        } catch (...) {
            return 0;
        }
    }

    ArchiJavaFxRuntime::RequestId ArchiJavaFxRuntime::Impl::dispatchWindow(
        std::string title, Arguments arguments, WindowMode mode) {
        if (arguments.size() > static_cast<std::size_t>(std::numeric_limits<jsize>::max() - 4)) {
            throw std::length_error("Too many arguments for JNI");
        }

        // Le runtime doit être démarré par initialize()/preloadAsync(). Un bouton
        // Creo ne démarre jamais la JVM : il ne fait qu'enregistrer une commande.
        std::lock_guard lock(mutex_);

        if (state_ != ArchiJavaFxRuntime::State::Starting && state_ != ArchiJavaFxRuntime::State::Ready) {
            return 0;
        }
        if (stopRequested_) {
            return 0;
        }

        constexpr std::size_t maxPendingCommands = 64;
        if (commands_.size() >= maxPendingCommands) {
            lastError_ = "Too many pending JavaFX window requests";
            return 0;
        }

        // Une seule modalité à la fois. Les fenêtres modeless sont également
        // refusées pendant cette barrière afin de garder un ordre UI déterministe.
        if (modalBusy_) {
            return 0;
        }

        // Le HWND n'est pas fourni par Creo Toolkit. Pour une modalité, le bouton
        // Creo est exécuté sur le thread UI de Creo : on identifie alors la fenêtre
        // hôte et on crée le helper Win32 sur CE thread. Pour une fenêtre modeless,
        // aucun HWND n'est nécessaire.
        if (mode == WindowMode::Modal && !hostModalGuard_.attachBestEffortFromCurrentThread()) {
            lastError_ = "Unable to identify the Creo host window on the current thread";
            return 0;
        }

        RequestId requestId = ++nextRequestId_;
        if (requestId == 0) {
            requestId = ++nextRequestId_;
        }

        WindowCommand command;
        command.kind = WindowCommand::Kind::OpenWindow;
        command.requestId = requestId;
        command.mode = mode;
        command.title = std::move(title);
        command.arguments = std::move(arguments);

        bool modalBlocked = false;
        if (mode == WindowMode::Modal) {
            // Aucun JNI et aucun appel Creo Toolkit ici. On ne fait que désactiver
            // les fenêtres Win32 de Creo avant de mettre la commande en queue.
            if (!hostModalGuard_.block(config_.modalBlockTimeout)) {
                lastError_ = "Unable to block Creo for modal JavaFX window";
                return 0;
            }
            modalBlocked = true;
            modalBusy_ = true;
        }

        try {
            requests_.emplace(requestId, RequestState{mode});
            commands_.push_back(std::move(command));
        } catch (...) {
            requests_.erase(requestId);
            if (mode == WindowMode::Modal) {
                modalBusy_ = false;
            }
            if (modalBlocked) {
                (void)hostModalGuard_.unblock(config_.modalBlockTimeout);
            }
            throw;
        }

        commandCondition_.notify_one();
        return requestId;
    }

    void ArchiJavaFxRuntime::Impl::completeProcessing(RequestId requestId, bool success, std::string message) {
        std::lock_guard lock(mutex_);
        const auto it = requests_.find(requestId);
        if (it == requests_.end() || !it->second.processing || stopRequested_) {
            return;
        }
        WindowCommand command;
        command.kind = WindowCommand::Kind::CompleteProcessing;
        command.requestId = requestId;
        command.mode = it->second.mode;
        command.processingSuccess = success;
        command.processingMessage = std::move(message);
        commands_.push_back(std::move(command));
        commandCondition_.notify_one();
    }

    void ArchiJavaFxRuntime::Impl::executeProcessingCompletion(JNIEnv* env, WindowCommand command) {
        try {
            jstring message = detail::newJavaString(env, command.processingMessage);
            if (completeProcessingMethod_ == nullptr) {
                throw std::runtime_error("JavaFxBridge.completeProcessing is missing");
            }
            env->CallStaticVoidMethod(
                bridgeClass_, completeProcessingMethod_,
                static_cast<jlong>(command.requestId),
                command.processingSuccess ? JNI_TRUE : JNI_FALSE,
                message);
            detail::throwIfJavaException(env, "completing JavaFX processing");
        } catch (...) {
            const std::string error = detail::exceptionMessage(std::current_exception());
            onWindowFailed(
                command.requestId,
                error.empty() ? "Unable to update JavaFX processing state" : error);
        }
    }

    void ArchiJavaFxRuntime::Impl::executeWindow(JNIEnv* env, WindowCommand command) {
        if (command.kind == WindowCommand::Kind::CompleteProcessing) {
            executeProcessingCompletion(env, std::move(command));
            return;
        }
        try {
            detail::LocalFrame localFrame(env, 16);

            jstring javaTitle = detail::newJavaString(env, command.title);
            jobjectArray javaArguments = env->NewObjectArray(
                static_cast<jsize>(command.arguments.size()), stringClass_, nullptr);
            if (javaArguments == nullptr) {
                detail::throwIfJavaException(env, "creating Java arguments");
                throw std::runtime_error("Unable to allocate Java argument array");
            }

            for (std::size_t i = 0; i < command.arguments.size(); ++i) {
                jstring value = detail::newJavaString(env, command.arguments[i]);
                env->SetObjectArrayElement(javaArguments, static_cast<jsize>(i), value);
                env->DeleteLocalRef(value);
                detail::throwIfJavaException(env, "copying Java argument");
            }

            {
                std::lock_guard lock(mutex_);
                if (stopRequested_) {
                    throw std::runtime_error("JavaFX runtime is shutting down");
                }
            }

            if (openWindowMethod_ == nullptr) {
                throw std::runtime_error("JavaFxBridge.openWindow is missing");
            }

            const jint mode = static_cast<jint>(command.mode);
            env->CallStaticVoidMethod(
                bridgeClass_, openWindowMethod_, javaTitle, javaArguments,
                mode, static_cast<jlong>(command.requestId));
            detail::throwIfJavaException(env, "opening JavaFX window");
        } catch (...) {
            const std::string errorMessage = detail::exceptionMessage(std::current_exception());
            onWindowFailed(command.requestId, errorMessage.empty() ? "Native JavaFX request failed" : errorMessage);
            return;
        }
    }

    void ArchiJavaFxRuntime::Impl::onWindowClosed(RequestId requestId) noexcept {
        onWindowResult(requestId, static_cast<int>(JavaFxResult::Status::Cancelled), {});
    }

    void ArchiJavaFxRuntime::Impl::onWindowResult(
        RequestId requestId, int status, std::vector<std::string> values) noexcept {
        WindowMode mode = WindowMode::Modeless;
        ResultCallback callback;
        bool modal = false;
        const bool accepted = status == static_cast<int>(JavaFxResult::Status::Accepted);

        {
            std::lock_guard lock(mutex_);
            const auto it = requests_.find(requestId);
            if (it == requests_.end()) {
                return;
            }
            mode = it->second.mode;
            modal = mode == WindowMode::Modal;
            if (accepted) {
                it->second.processing = true;
            } else {
                requests_.erase(it);
                if (modal) {
                    modalBusy_ = false;
                }
            }
            try {
                callback = resultCallback_;
            } catch (...) {
                lastError_ = detail::exceptionMessage(std::current_exception());
            }
        }

        if (!accepted && modal) {
            // Never block the JavaFX Application Thread waiting for the Creo
            // owner thread. The host guard posts the unblock to its owner thread.
            hostModalGuard_.unblockAsync();
        }

        if (!callback) {
            return;
        }

        JavaFxResult result;
        result.requestId = requestId;
        result.mode = mode;
        if (status < static_cast<int>(JavaFxResult::Status::Accepted) ||
            status > static_cast<int>(JavaFxResult::Status::Failed)) {
            result.status = JavaFxResult::Status::Failed;
            result.error = "Unknown JavaFX result status: " + std::to_string(status);
            result.values.clear();
        } else {
            result.status = static_cast<JavaFxResult::Status>(status);
            result.values = std::move(values);
        }

        try {
            callback(std::move(result));
        } catch (...) {
            std::lock_guard lock(mutex_);
            lastError_ = detail::exceptionMessage(std::current_exception());
        }
    }

    void ArchiJavaFxRuntime::Impl::onWindowProcessingFinished(
        RequestId requestId, bool success) noexcept {
        WindowMode mode = WindowMode::Modeless;
        bool modal = false;
        {
            std::lock_guard lock(mutex_);
            const auto it = requests_.find(requestId);
            if (it == requests_.end()) {
                return;
            }
            mode = it->second.mode;
            modal = mode == WindowMode::Modal;
            if (!success) {
                it->second.processing = false;
                return;
            }
            requests_.erase(it);
            if (modal) {
                modalBusy_ = false;
            }
        }
        if (modal) {
            // Completion is called from the JavaFX Application Thread. Releasing
            // Creo must therefore remain asynchronous.
            hostModalGuard_.unblockAsync();
        }
    }

    void ArchiJavaFxRuntime::Impl::onWindowFailed(RequestId requestId, std::string message) noexcept {
        WindowMode mode = WindowMode::Modeless;
        ResultCallback callback;
        bool modal = false;
        const std::string errorMessage = message.empty() ? "Unknown JavaFX error" : message;

        {
            std::lock_guard lock(mutex_);
            const auto it = requests_.find(requestId);
            if (it == requests_.end()) {
                return;
            }
            mode = it->second.mode;
            modal = mode == WindowMode::Modal;
            requests_.erase(it);
            if (modal) {
                modalBusy_ = false;
            }
            lastError_ = errorMessage;
            try {
                callback = resultCallback_;
            } catch (...) {
                lastError_ = detail::exceptionMessage(std::current_exception());
            }
        }

        if (modal) {
            (void)hostModalGuard_.unblock(config_.modalBlockTimeout);
        }

        if (!callback) {
            return;
        }

        JavaFxResult result;
        result.requestId = requestId;
        result.mode = mode;
        result.status = JavaFxResult::Status::Failed;
        result.error = errorMessage;

        try {
            callback(std::move(result));
        } catch (...) {
            std::lock_guard lock(mutex_);
            lastError_ = detail::exceptionMessage(std::current_exception());
        }
    }

    void ArchiJavaFxRuntime::Impl::failAllRequests(const std::exception_ptr& error) noexcept {
        std::lock_guard lock(mutex_);
        if (error) {
            const std::string message = detail::exceptionMessage(error);
            if (!message.empty()) {
                lastError_ = message;
            }
        }
        commands_.clear();
        requests_.clear();
        modalBusy_ = false;
        hostModalGuard_.unblockAsync();
    }

    void ArchiJavaFxRuntime::Impl::cancelPendingCommandsLocked(const std::exception_ptr& error) noexcept {
        if (error) {
            const std::string message = detail::exceptionMessage(error);
            if (!message.empty()) {
                lastError_ = message;
            }
        }
        commands_.clear();
        requests_.clear();
        modalBusy_ = false;
    }

    void ArchiJavaFxRuntime::Impl::setResultCallback(ResultCallback callback) {
        std::lock_guard lock(mutex_);
        resultCallback_ = std::move(callback);
    }

    void ArchiJavaFxRuntime::Impl::shutdown() noexcept {
        std::unique_lock lifecycleLock(lifecycleMutex_);

        if (jvmThread_.joinable() && jvmThread_.get_id() == std::this_thread::get_id()) {
            // Ne jamais join soi-même. Le runtime est conçu pour que destroy() soit
            // appelé depuis le plugin/Creo, pas depuis un callback JNI.
            lastError_ = "shutdown called from the JVM worker thread";
            return;
        }

        {
            std::unique_lock lock(mutex_);
            if (!jvmThread_.joinable()) {
                stopRequested_ = true;
                modalBusy_ = false;
                commands_.clear();
                requests_.clear();
                lock.unlock();
                hostModalGuard_.unblockAsync();
                jvmLoader_.unload();
                return;
            }

            stopRequested_ = true;
            if (state_ == ArchiJavaFxRuntime::State::Ready || state_ == ArchiJavaFxRuntime::State::Starting) {
                state_ = ArchiJavaFxRuntime::State::Stopping;
            }
            cancelPendingCommandsLocked(makeError("JavaFX runtime is shutting down"));
        }

        hostModalGuard_.unblockAsync();
        commandCondition_.notify_all();
        readyCondition_.notify_all();

        if (jvmThread_.joinable()) {
            jvmThread_.join();
        }

        // DestroyJavaVM a terminé avant le retour de jvmMain(). Il est désormais
        // sûr de décharger jvm.dll.
        jvmLoader_.unload();
        hostModalGuard_.unblockAsync();
    }

    jclass ArchiJavaFxRuntime::Impl::defineEmbeddedClasses(JNIEnv* env) {
        if (config_.embeddedClasses.empty()) {
            return nullptr;
        }

        jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
        detail::throwIfJavaException(env, "loading java.lang.ClassLoader");
        if (classLoaderClass == nullptr) {
            throw std::runtime_error("java.lang.ClassLoader not found");
        }

        const jmethodID getSystemClassLoader = env->GetStaticMethodID(
            classLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
        detail::throwIfJavaException(env, "resolving ClassLoader.getSystemClassLoader");
        if (getSystemClassLoader == nullptr) {
            throw std::runtime_error("ClassLoader.getSystemClassLoader not found");
        }

        jobject systemClassLoader = env->CallStaticObjectMethod(classLoaderClass, getSystemClassLoader);
        detail::throwIfJavaException(env, "getting the system classloader");
        if (systemClassLoader == nullptr) {
            throw std::runtime_error("System classloader is unavailable");
        }

        jclass urlClass = env->FindClass("java/net/URL");
        jclass urlClassLoaderClass = env->FindClass("java/net/URLClassLoader");
        detail::throwIfJavaException(env, "loading java.net.URLClassLoader");
        if (urlClass == nullptr || urlClassLoaderClass == nullptr) {
            throw std::runtime_error("URLClassLoader is unavailable");
        }

        jobjectArray noUrls = env->NewObjectArray(0, urlClass, nullptr);
        const jmethodID constructor = env->GetMethodID(
            urlClassLoaderClass, "<init>", "([Ljava/net/URL;Ljava/lang/ClassLoader;)V");
        detail::throwIfJavaException(env, "resolving URLClassLoader constructor");
        if (noUrls == nullptr || constructor == nullptr) {
            throw std::runtime_error("Unable to create isolated classloader");
        }

        jobject isolatedLoader = env->NewObject(urlClassLoaderClass, constructor, noUrls, systemClassLoader);
        detail::throwIfJavaException(env, "creating isolated classloader");
        if (isolatedLoader == nullptr) {
            throw std::runtime_error("Isolated classloader is unavailable");
        }

        embeddedClassLoader_ = env->NewGlobalRef(isolatedLoader);
        if (embeddedClassLoader_ == nullptr) {
            detail::throwIfJavaException(env, "keeping isolated classloader");
            throw std::runtime_error("Unable to keep isolated classloader");
        }

        jclass embeddedBridge = nullptr;
        for (const auto& embedded : config_.embeddedClasses) {
            jclass defined = env->DefineClass(embedded.binaryName.c_str(), embeddedClassLoader_,
                reinterpret_cast<const jbyte*>(embedded.bytecode.data()), static_cast<jsize>(embedded.bytecode.size()));
            const std::string operation = "injecting " + embedded.binaryName;
            detail::throwIfJavaException(env, operation.c_str());
            if (defined == nullptr) {
                throw std::runtime_error("DefineClass failed: " + embedded.binaryName);
            }

            if (embedded.binaryName == config_.bridgeClass) {
                if (embeddedBridge != nullptr) {
                    env->DeleteLocalRef(embeddedBridge);
                }
                embeddedBridge = defined;
            } else {
                env->DeleteLocalRef(defined);
            }
        }

        env->DeleteLocalRef(isolatedLoader);
        env->DeleteLocalRef(noUrls);
        env->DeleteLocalRef(urlClassLoaderClass);
        env->DeleteLocalRef(urlClass);
        env->DeleteLocalRef(systemClassLoader);
        env->DeleteLocalRef(classLoaderClass);
        return embeddedBridge;
    }

    void ArchiJavaFxRuntime::Impl::registerNativeCallbacks(JNIEnv* env) {
        static JNINativeMethod methods[] = {
            { const_cast<char*>("nativeWindowClosed"), const_cast<char*>("(J)V"),
                reinterpret_cast<void*>(&ArchiJavaFxRuntime::Impl::nativeWindowClosed) },
            { const_cast<char*>("nativeWindowResult"), const_cast<char*>("(JI[Ljava/lang/String;)V"),
                reinterpret_cast<void*>(&ArchiJavaFxRuntime::Impl::nativeWindowResult) },
            { const_cast<char*>("nativeWindowFailed"), const_cast<char*>("(JLjava/lang/String;)V"),
                reinterpret_cast<void*>(&ArchiJavaFxRuntime::Impl::nativeWindowFailed) },
            { const_cast<char*>("nativeProcessingFinished"), const_cast<char*>("(JZ)V"),
                reinterpret_cast<void*>(&ArchiJavaFxRuntime::Impl::nativeProcessingFinished) },
        };

        const jint result = env->RegisterNatives(bridgeClass_, methods, 4);
        detail::throwIfJavaException(env, "registering JavaFX native callbacks");
        if (result != JNI_OK) {
            throw std::runtime_error("RegisterNatives failed");
        }
    }

    void ArchiJavaFxRuntime::Impl::unregisterNativeCallbacks(JNIEnv* env) noexcept {
        if (env != nullptr && bridgeClass_ != nullptr) {
            env->UnregisterNatives(bridgeClass_);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
            }
        }
    }

    void JNICALL ArchiJavaFxRuntime::Impl::nativeWindowClosed(JNIEnv*, jclass, jlong requestId) {
        auto* runtime = static_cast<Impl*>(gActiveRuntime.load(std::memory_order_acquire));
        if (runtime == nullptr) {
            return;
        }
        runtime->onWindowClosed(static_cast<RequestId>(requestId));
    }

    void JNICALL ArchiJavaFxRuntime::Impl::nativeWindowResult(
        JNIEnv* env, jclass, jlong requestId, jint status, jobjectArray values) {
        auto* runtime = static_cast<Impl*>(gActiveRuntime.load(std::memory_order_acquire));
        if (runtime == nullptr) {
            return;
        }
        runtime->onWindowResult(
            static_cast<RequestId>(requestId), static_cast<int>(status), javaStringArrayToUtf8NoThrow(env, values));
    }

    void JNICALL ArchiJavaFxRuntime::Impl::nativeProcessingFinished(JNIEnv*, jclass, jlong requestId, jboolean success) {
        auto* runtime = static_cast<Impl*>(gActiveRuntime.load(std::memory_order_acquire));
        if (runtime == nullptr) {
            return;
        }
        runtime->onWindowProcessingFinished(static_cast<RequestId>(requestId), success == JNI_TRUE);
    }

    void JNICALL ArchiJavaFxRuntime::Impl::nativeWindowFailed(JNIEnv* env, jclass, jlong requestId, jstring message) {
        auto* runtime = static_cast<Impl*>(gActiveRuntime.load(std::memory_order_acquire));
        if (runtime == nullptr) {
            return;
        }
        std::string error = javaStringToUtf8NoThrow(env, message);
        if (error.empty()) {
            error = "Unknown JavaFX error";
        }
        runtime->onWindowFailed(static_cast<RequestId>(requestId), std::move(error));
    }

    void ArchiJavaFxRuntime::Impl::jvmMain() {
        try {
            createJvmAndWarmUpJavaFx();
            gActiveRuntime.store(this, std::memory_order_release);

            {
                std::lock_guard lock(mutex_);
                if (stopRequested_) {
                    state_ = ArchiJavaFxRuntime::State::Stopping;
                } else {
                    state_ = ArchiJavaFxRuntime::State::Ready;
                }
            }
            readyCondition_.notify_all();

            JNIEnv* env = nullptr;
            if (vm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_10) != JNI_OK || env == nullptr) {
                throw std::runtime_error("JVM owner thread lost its JNIEnv");
            }

            for (;;) {
                WindowCommand command;
                {
                    std::unique_lock lock(mutex_);
                    commandCondition_.wait(lock, [this] { return stopRequested_ || !commands_.empty(); });
                    if (stopRequested_) {
                        cancelPendingCommandsLocked(makeError("JavaFX runtime is shutting down"));
                        break;
                    }

                    command = std::move(commands_.front());
                    commands_.pop_front();
                }

                executeWindow(env, std::move(command));
            }

            shutdownOnJvmThread(env);
            cleanupJavaRefs(env);

            JavaVM* vmToDestroy = vm_;
            vm_ = nullptr;
            const jint destroyResult = vmToDestroy->DestroyJavaVM();
            if (destroyResult != JNI_OK) {
                throw std::runtime_error("DestroyJavaVM failed, code " + std::to_string(destroyResult));
            }

            gActiveRuntime.store(nullptr, std::memory_order_release);

            {
                std::lock_guard lock(mutex_);
                state_ = ArchiJavaFxRuntime::State::Terminated;
            }
            readyCondition_.notify_all();
        } catch (...) {
            const std::exception_ptr error = std::current_exception();
            gActiveRuntime.store(nullptr, std::memory_order_release);
            failAllRequests(error);
            cleanupFailedJvm();
            {
                std::lock_guard lock(mutex_);
                if (lastError_.empty()) {
                    lastError_ = detail::exceptionMessage(error);
                }
                state_ = ArchiJavaFxRuntime::State::Failed;
            }
            readyCondition_.notify_all();
            commandCondition_.notify_all();
        }
    }

    void ArchiJavaFxRuntime::Impl::createJvmAndWarmUpJavaFx() {
        jvmLoader_.load(config_.jvmPath, config_.jreEnvironmentVariable);

        if (config_.jvmOptions.size() >= static_cast<std::size_t>(std::numeric_limits<jint>::max())) {
            throw std::length_error("Too many JVM options");
        }

        JavaVMInitArgs arguments{};
        arguments.version = JNI_VERSION_10;
        arguments.ignoreUnrecognized = JNI_FALSE;

        std::vector<std::string> optionStrings;
        optionStrings.reserve(config_.jvmOptions.size() + 1);
        optionStrings.emplace_back("-Djava.class.path=" + detail::expandClassPath(config_.classPath));
        optionStrings.insert(optionStrings.end(), config_.jvmOptions.begin(), config_.jvmOptions.end());

        std::vector<JavaVMOption> options(optionStrings.size());
        for (std::size_t i = 0; i < optionStrings.size(); ++i) {
            options[i].optionString = optionStrings[i].data();
            options[i].extraInfo = nullptr;
        }
        arguments.nOptions = static_cast<jint>(options.size());
        arguments.options = options.data();

        JavaVM* createdVm = nullptr;
        JNIEnv* env = nullptr;

        {
            std::lock_guard creationLock(gJvmCreationMutex);
            JavaVM* existingVm = nullptr;
            jsize existingCount = 0;
            const jint vmQueryResult = jvmLoader_.getCreatedJavaVms(&existingVm, 1, &existingCount);
            if (vmQueryResult != JNI_OK) {
                throw std::runtime_error("Unable to query existing JVMs");
            }
            if (existingCount != 0) {
                throw std::runtime_error("A JVM already exists in this process");
            }

            const jint creationResult = jvmLoader_.createJavaVm(&createdVm, &env, &arguments);
            if (creationResult != JNI_OK) {
                throw std::runtime_error("JNI_CreateJavaVM failed, code " + std::to_string(creationResult));
            }

            // Store immediately after a successful create. Any later exception can
            // therefore clean up the JVM deterministically.
            vm_ = createdVm;
        }

        if (createdVm == nullptr || env == nullptr) {
            throw std::runtime_error("JNI_CreateJavaVM returned JNI_OK with null VM/JNIEnv");
        }

        jclass localBridge = defineEmbeddedClasses(env);
        if (localBridge == nullptr) {
            localBridge = env->FindClass(config_.bridgeClass.c_str());
            detail::throwIfJavaException(env, "loading JavaFxBridge");
        }
        if (localBridge == nullptr) {
            throw std::runtime_error("JavaFxBridge class not found");
        }

        bridgeClass_ = static_cast<jclass>(env->NewGlobalRef(localBridge));
        env->DeleteLocalRef(localBridge);
        if (bridgeClass_ == nullptr) {
            detail::throwIfJavaException(env, "keeping JavaFxBridge");
            throw std::runtime_error("Unable to keep JavaFxBridge");
        }

        jclass localString = env->FindClass("java/lang/String");
        detail::throwIfJavaException(env, "loading java.lang.String");
        if (localString == nullptr) {
            throw std::runtime_error("java.lang.String not found");
        }
        stringClass_ = static_cast<jclass>(env->NewGlobalRef(localString));
        env->DeleteLocalRef(localString);
        if (stringClass_ == nullptr) {
            detail::throwIfJavaException(env, "keeping java.lang.String");
            throw std::runtime_error("Unable to keep java.lang.String");
        }

        initializeMethod_ = env->GetStaticMethodID(bridgeClass_, "initialize", "()V");
        openWindowMethod_ = env->GetStaticMethodID(
            bridgeClass_, "openWindow", "(Ljava/lang/String;[Ljava/lang/String;IJ)V");
        completeProcessingMethod_ = env->GetStaticMethodID(
            bridgeClass_, "completeProcessing", "(JZLjava/lang/String;)V");
        shutdownMethod_ = env->GetStaticMethodID(bridgeClass_, "shutdown", "()V");
        detail::throwIfJavaException(env, "resolving JavaFxBridge methods");

        if (initializeMethod_ == nullptr || openWindowMethod_ == nullptr || completeProcessingMethod_ == nullptr || shutdownMethod_ == nullptr) {
            throw std::runtime_error("JavaFxBridge method missing");
        }

        registerNativeCallbacks(env);

        env->CallStaticVoidMethod(bridgeClass_, initializeMethod_);
        detail::throwIfJavaException(env, "initializing JavaFX");
    }

    void ArchiJavaFxRuntime::Impl::shutdownOnJvmThread(JNIEnv* env) noexcept {
        if (bridgeClass_ == nullptr || shutdownMethod_ == nullptr) {
            return;
        }

        try {
            env->CallStaticVoidMethod(bridgeClass_, shutdownMethod_);
            detail::throwIfJavaException(env, "stopping JavaFX");
        } catch (...) {
            const std::exception_ptr error = std::current_exception();
            std::lock_guard lock(mutex_);
            lastError_ = detail::exceptionMessage(error);
        }
    }

    void ArchiJavaFxRuntime::Impl::cleanupJavaRefs(JNIEnv* env) noexcept {
        if (env == nullptr) {
            return;
        }

        unregisterNativeCallbacks(env);

        if (bridgeClass_ != nullptr) {
            env->DeleteGlobalRef(bridgeClass_);
            bridgeClass_ = nullptr;
        }
        if (stringClass_ != nullptr) {
            env->DeleteGlobalRef(stringClass_);
            stringClass_ = nullptr;
        }
        if (embeddedClassLoader_ != nullptr) {
            env->DeleteGlobalRef(embeddedClassLoader_);
            embeddedClassLoader_ = nullptr;
        }

        initializeMethod_ = nullptr;
        openWindowMethod_ = nullptr;
        completeProcessingMethod_ = nullptr;
        shutdownMethod_ = nullptr;
    }

    void ArchiJavaFxRuntime::Impl::cleanupFailedJvm() noexcept {
        if (vm_ == nullptr) {
            return;
        }

        JavaVM* vm = vm_;
        JNIEnv* env = nullptr;
        if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_10) == JNI_OK && env != nullptr) {
            if (bridgeClass_ != nullptr && shutdownMethod_ != nullptr) {
                env->CallStaticVoidMethod(bridgeClass_, shutdownMethod_);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                }
            }
            cleanupJavaRefs(env);
        }

        vm->DestroyJavaVM();
        vm_ = nullptr;
    }

}    // namespace jnifx