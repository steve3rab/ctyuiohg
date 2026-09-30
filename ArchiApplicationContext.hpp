#pragma once

#include "ArchiJavaFxRuntime.hpp"
#include "ArchiPropertyUtils.hpp"

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace archi {

enum class JavaFxAction : std::uint8_t {
    CreatePart,
    CreateAssembly
};

/**
 * Process-wide application/session state.
 *
 * Lifetime:
 *   - initialized on first use;
 *   - lives until process termination;
 *   - immutable application paths are initialized once;
 *   - mutable session state is protected by mutex_.
 *
 * Important:
 *   This class stores native application state only. It must not own
 *   Creo TOOLKIT handles or JNI/JavaFX objects because those objects have
 *   thread/lifecycle ownership rules of their own.
 */
class ArchiApplicationContext final {
public:
    using RequestId = jnifx::ArchiJavaFxRuntime::RequestId;

    ArchiApplicationContext(const ArchiApplicationContext&) = delete;
    ArchiApplicationContext& operator=(const ArchiApplicationContext&) = delete;
    ArchiApplicationContext(ArchiApplicationContext&&) = delete;
    ArchiApplicationContext& operator=(ArchiApplicationContext&&) = delete;

    static ArchiApplicationContext& instance() noexcept
    {
        static ArchiApplicationContext context;
        return context;
    }

    const std::filesystem::path& applicationHome() const noexcept
    {
        return applicationHome_;
    }

    const std::filesystem::path& templateDirectory() const noexcept
    {
        return templateDirectory_;
    }

    bool registerPendingAction(
        RequestId requestId,
        JavaFxAction action)
    {
        if (requestId == 0) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        return pendingActions_.emplace(requestId, action).second;
    }

    std::optional<JavaFxAction> pendingAction(
        RequestId requestId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto it = pendingActions_.find(requestId);
        if (it == pendingActions_.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    bool removePendingAction(RequestId requestId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return pendingActions_.erase(requestId) != 0;
    }

private:
    ArchiApplicationContext() :
        applicationHome_(
            ArchiPropertyUtils::environmentPath(
                L"Archi_TOOLS")),
        templateDirectory_(
            applicationHome_ / "templates")
    {
    }

    const std::filesystem::path applicationHome_;
    const std::filesystem::path templateDirectory_;

    mutable std::mutex mutex_;
    std::unordered_map<RequestId, JavaFxAction> pendingActions_;
};

} // namespace archi
