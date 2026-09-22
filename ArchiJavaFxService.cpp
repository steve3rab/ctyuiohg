#include "ArchiJavaFxService.hpp"

#ifndef _WIN32
#error ArchiJavaFxService is Windows-only.
#endif

#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "ArchiPropertyUtils.hpp"

namespace {
    constexpr const char* jreHomeVariable = "Archi_JAVA_HOME";
    constexpr const char* javaClassName = "com/thales/hwb/Archi/launcher/ArchiCreoJniLauncher";

    std::mutex& lifecycleMutex() {
        static std::mutex mutex;
        return mutex;
    }

}

jnifx::ArchiJavaFxRuntime::Config ArchiJavaFxService::createConfig() {
    const std::filesystem::path applicationHome = ArchiPropertyUtils::environmentPath(L"Archi_TOOLS");
    const std::filesystem::path libs = applicationHome / "lib";

    jnifx::ArchiJavaFxRuntime::Config config;
    config.classPath = libs.u8string() + R"(\*)";
    config.bridgeClass = javaClassName;
    config.jvmOptions = {"-Dfile.encoding=UTF-8"};
    config.modalBlockTimeout = std::chrono::seconds(2);
    config.jreEnvironmentVariable = jreHomeVariable;
    return config;
}

ArchiJavaFxService::ArchiJavaFxService() : runtime_(createConfig()) {
    // Le JVM et JavaFX sont préchargés dès la création du service, donc avant le
    // premier clic utilisateur dans Creo.
    runtime_.preloadAsync();
}

ArchiJavaFxService::~ArchiJavaFxService() {
    runtime_.shutdown();
}

void ArchiJavaFxService::Deleter::operator()(ArchiJavaFxService* service) const noexcept {
    delete service;
}

ArchiJavaFxService::InstancePtr& ArchiJavaFxService::activeInstanceStorage() {
    static InstancePtr instance;
    return instance;
}

bool ArchiJavaFxService::initialize() noexcept {
    std::lock_guard lock(lifecycleMutex());
    auto& activeInstance = activeInstanceStorage();

    if (activeInstance != nullptr) {
        return true;
    }

    try {
        activeInstance.reset(new ArchiJavaFxService());
        return true;
    } catch (...) {
        activeInstance.reset();
        return false;
    }
}

void ArchiJavaFxService::destroy() noexcept {
    std::lock_guard lock(lifecycleMutex());
    activeInstanceStorage().reset();
}

ArchiJavaFxService& ArchiJavaFxService::instance() {
    auto& activeInstance = activeInstanceStorage();
    if (activeInstance == nullptr) {
        throw std::logic_error("ArchiJavaFxService is not initialized");
    }
    return *activeInstance;
}

bool ArchiJavaFxService::openWindow(
    std::string title,
    std::vector<std::string> arguments) noexcept {
    try {
        return runtime_.tryOpenWindow(std::move(title), std::move(arguments));
    } catch (...) {
        return false;
    }
}

bool ArchiJavaFxService::openWindow(
    std::string title,
    std::initializer_list<std::string> arguments) noexcept {
    return openWindow(std::move(title), std::vector<std::string>(arguments));
}

bool ArchiJavaFxService::openModalWindow(
    std::string title,
    std::vector<std::string> arguments) noexcept {
    try {
        return runtime_.tryOpenModalWindow(std::move(title), std::move(arguments));
    } catch (...) {
        return false;
    }
}

bool ArchiJavaFxService::openModalWindow(
    std::string title,
    std::initializer_list<std::string> arguments) noexcept {
    return openModalWindow(std::move(title), std::vector<std::string>(arguments));
}
