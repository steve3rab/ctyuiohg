#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "ArchiJavaFxRuntime.hpp"

class ArchiJavaFxService final {
  public:
    ArchiJavaFxService(const ArchiJavaFxService&) = delete;
    ArchiJavaFxService& operator=(const ArchiJavaFxService&) = delete;
    ArchiJavaFxService(ArchiJavaFxService&&) = delete;
    ArchiJavaFxService& operator=(ArchiJavaFxService&&) = delete;

    // Lifecycle du plugin : appeler dans user_initialize()/user_terminate().
    static bool initialize() noexcept;
    static void destroy() noexcept;
    static ArchiJavaFxService& instance();

    // Modeless : le callback Creo rend immédiatement la main et Creo reste utilisable.
    bool openWindow(std::string title, std::vector<std::string> arguments = {}) noexcept;
    bool openWindow(std::string title, std::initializer_list<std::string> arguments) noexcept;

    // Modal : le callback Creo ne reste pas bloqué, mais les fenêtres Creo sont
    // désactivées jusqu'à la fermeture de la fenêtre JavaFX.
    bool openModalWindow(std::string title, std::vector<std::string> arguments = {}) noexcept;
    bool openModalWindow(std::string title, std::initializer_list<std::string> arguments) noexcept;

  private:
    ArchiJavaFxService();
    ~ArchiJavaFxService();

    struct Deleter {
        void operator()(ArchiJavaFxService* service) const noexcept;
    };

    using InstancePtr = std::unique_ptr<ArchiJavaFxService, Deleter>;

    static InstancePtr& activeInstanceStorage();
    static jnifx::ArchiJavaFxRuntime::Config createConfig();

    jnifx::ArchiJavaFxRuntime runtime_;
};
