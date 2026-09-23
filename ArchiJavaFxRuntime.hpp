#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace jnifx {

class ArchiJavaFxRuntime final {
public:
    using Arguments = std::vector<std::string>;
    using RequestId = std::uint64_t;

    struct JavaFxResult final {
        enum class Status : std::uint8_t { Accepted, Cancelled, Failed };
        Status status = Status::Failed;
        std::vector<std::string> values;
        std::string error;
    };

    using ResultCallback = std::function<void(RequestId, JavaFxResult)>;

    struct EmbeddedClass { std::string binaryName; std::vector<std::uint8_t> bytecode; };
    enum class WindowMode : std::uint8_t { Modeless = 0, Modal = 1 };

    struct Config {
        std::string classPath;
        std::vector<std::string> jvmOptions;
        std::string bridgeClass;
        std::chrono::milliseconds modalBlockTimeout{2000};
        std::string jvmPath;
        std::string jreEnvironmentVariable = "HCP_JAVA_HOME";
        std::vector<EmbeddedClass> embeddedClasses;
    };

    explicit ArchiJavaFxRuntime(Config config);
    ~ArchiJavaFxRuntime();

    ArchiJavaFxRuntime(const ArchiJavaFxRuntime&) = delete;
    ArchiJavaFxRuntime& operator=(const ArchiJavaFxRuntime&) = delete;
    ArchiJavaFxRuntime(ArchiJavaFxRuntime&&) = delete;
    ArchiJavaFxRuntime& operator=(ArchiJavaFxRuntime&&) = delete;

    void preloadAsync();

    bool tryOpenWindow(std::string title, Arguments arguments = {}, ResultCallback callback = {});
    bool tryOpenModalWindow(std::string title, Arguments arguments = {}, ResultCallback callback = {});

    void shutdown() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace jnifx
