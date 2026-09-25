#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "ArchiCreoUiDispatcher.hpp"
#include "ArchiJavaFxRuntime.hpp"

class ArchiJavaFxService final {
  public:
    using RequestId = jnifx::ArchiJavaFxRuntime::RequestId;
    using ResultCallback = jnifx::ArchiJavaFxRuntime::ResultCallback;
    ArchiJavaFxService(const ArchiJavaFxService&) = delete;
    ArchiJavaFxService& operator=(const ArchiJavaFxService&) = delete;
    ArchiJavaFxService(ArchiJavaFxService&&) = delete;
    ArchiJavaFxService& operator=(ArchiJavaFxService&&) = delete;

    // Lifecycle du plugin : appeler dans user_initialize()/user_terminate().
    static bool initialize() noexcept;
    static void destroy() noexcept;
    static ArchiJavaFxService& instance();

    // Modeless : le callback Creo rend immédiatement la main et Creo reste utilisable.
    RequestId openWindow(std::string title, std::vector<std::string> arguments = {}) noexcept;
    RequestId openWindow(std::string title, std::initializer_list<std::string> arguments) noexcept;

    // Modal : le callback Creo ne reste pas bloqué, mais les fenêtres Creo sont
    // désactivées jusqu'à la fermeture de la fenêtre JavaFX.
    RequestId openModalWindow(std::string title, std::vector<std::string> arguments = {}) noexcept;
    RequestId openModalWindow(std::string title, std::initializer_list<std::string> arguments) noexcept;

    void setResultCallback(ResultCallback callback);
    void completeProcessing(RequestId requestId, bool success, std::string message = {});

  private:
    ArchiJavaFxService();
    ~ArchiJavaFxService();

    struct Deleter {
        void operator()(ArchiJavaFxService* service) const noexcept;
    };

    using InstancePtr = std::unique_ptr<ArchiJavaFxService, Deleter>;

    static InstancePtr& activeInstanceStorage();
    static jnifx::ArchiJavaFxRuntime::Config createConfig();

    // Déclaré avant runtime_ : il reste vivant pendant tout le shutdown du
    // runtime, y compris pendant les derniers callbacks JNI.
    jnifx::detail::ArchiCreoUiDispatcher dispatcher_;
    jnifx::ArchiJavaFxRuntime runtime_;
};
