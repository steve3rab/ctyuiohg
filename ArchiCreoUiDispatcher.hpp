#pragma once

#ifndef _WIN32
#error ArchiCreoUiDispatcher is Windows-only.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cstddef>
#include <functional>
#include <memory>
#include <windows.h>

namespace jnifx::detail {

/**
 * Dispatches work back to the thread on which attachToCurrentThread() was called.
 *
 * Intended for Creo TOOLKIT integration:
 * - attach from user_initialize() / a Creo callback thread;
 * - post small tasks from JNI/JavaFX callbacks;
 * - the task is executed on the owner thread;
 * - no worker thread is created and no Creo TOOLKIT call is made from the
 *   JavaFX/JVM thread.
 *
 * Tasks must not retain references to stack variables or to this dispatcher.
 * post() copies/moves the callable into an internal queue.
 */
class ArchiCreoUiDispatcher final {
  public:
    using Task = std::function<void()>;

    ArchiCreoUiDispatcher() = default;
    ~ArchiCreoUiDispatcher();

    ArchiCreoUiDispatcher(const ArchiCreoUiDispatcher&) = delete;
    ArchiCreoUiDispatcher& operator=(const ArchiCreoUiDispatcher&) = delete;
    ArchiCreoUiDispatcher(ArchiCreoUiDispatcher&&) = delete;
    ArchiCreoUiDispatcher& operator=(ArchiCreoUiDispatcher&&) = delete;

    /**
     * Must be called from the Creo UI/callback thread.
     * Safe to call repeatedly from that same thread.
     */
    bool attachToCurrentThread() noexcept;

    /**
     * Thread-safe. May be called from JNI/JavaFX or any other thread.
     * Returns false when the dispatcher is not accepting work.
     */
    bool post(Task task) noexcept;

    /**
     * Stops accepting new work and synchronously drains/destroys the helper
     * window on the owner thread when possible. Safe from any thread.
     */
    void shutdown() noexcept;

    bool isAttached() const noexcept;
    DWORD ownerThreadId() const noexcept { return ownerThreadId_; }
    HWND helperWindow() const noexcept { return helperWindow_; }

  private:
    struct State;

    static constexpr UINT kExecuteMessage = WM_APP + 0x620;
    static constexpr UINT kDestroyMessage = WM_APP + 0x621;
    static constexpr wchar_t kWindowClassName[] = L"ArchiCreoUiDispatcher";

    static LRESULT CALLBACK wndProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    bool createHelperWindow() noexcept;

    std::shared_ptr<State> state_;
    HWND helperWindow_ = nullptr;
    DWORD ownerThreadId_ = 0;
};

} // namespace jnifx::detail
