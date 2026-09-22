#pragma once

#ifndef _WIN32
#error ArchiWindowsHostModalGuard supports Windows only.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <chrono>
#include <vector>
#include <windows.h>

namespace jnifx::detail {

    // Bridges the JVM/JavaFX side to the Creo UI thread without calling Creo
    // TOOLKIT from a worker. A message-only helper window belongs to the Creo UI
    // thread and owns its state, so destroying the C++ guard from another thread
    // cannot leave a window procedure pointing at a dead C++ object.
    class ArchiWindowsHostModalGuard final {
      public:
        ArchiWindowsHostModalGuard() = default;
        explicit ArchiWindowsHostModalGuard(HWND hostWindow);
        ~ArchiWindowsHostModalGuard();

        ArchiWindowsHostModalGuard(const ArchiWindowsHostModalGuard&) = delete;
        ArchiWindowsHostModalGuard& operator=(const ArchiWindowsHostModalGuard&) = delete;

        bool attach(HWND hostWindow);
        // À appeler depuis le callback Creo (thread UI de Creo). Ne dépend pas
        // d'un HWND exposé par Creo Toolkit.
        bool attachBestEffortFromCurrentThread() noexcept;
        void detach() noexcept;

        // Called by the JVM worker before the modal JavaFX request is posted.
        // The call returns only after the Creo UI thread has applied the block,
        // or after the timeout expires.
        bool block(std::chrono::milliseconds timeout);

        // Safe from any thread; never waits for the Creo UI thread.
        bool unblock(std::chrono::milliseconds timeout) noexcept;
        void unblockAsync() noexcept;

        // Cleanup convenience for code already running on the Creo UI thread.
        void forceUnblockOnOwnerThread() noexcept;

        HWND hostWindow() const noexcept { return hostWindow_; }
        HWND helperWindow() const noexcept { return helperWindow_; }

      private:
        struct DisabledWindow final {
            HWND hwnd = nullptr;
            bool wasEnabled = true;
        };

        struct State final {
            HWND hostWindow = nullptr;
            DWORD ownerThreadId = 0;
            bool blocked = false;
            std::vector<DisabledWindow> disabledWindows;
            HWND previousForegroundWindow = nullptr;
            HWND previousActiveWindow = nullptr;
            HWND previousFocusWindow = nullptr;
        };

        static constexpr UINT kBlockMessage = WM_APP + 0x513;
        static constexpr UINT kUnblockMessage = WM_APP + 0x514;
        static constexpr UINT kDestroyMessage = WM_APP + 0x515;
        static constexpr wchar_t kWindowClassName[] = L"ArchiJavaFxHostModalGuard";

        static LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        static BOOL CALLBACK enumTopLevelWindow(HWND hwnd, LPARAM lParam);
        static bool belongsToHostWindow(const State& state, HWND hwnd) noexcept;
        static bool applyBlockOnOwnerThread(State& state) noexcept;
        static void applyUnblockOnOwnerThread(State& state) noexcept;
        static void restoreHostActivationOnOwnerThread(State& state) noexcept;

        bool createHelperWindow();

        HWND hostWindow_ = nullptr;
        HWND helperWindow_ = nullptr;
        DWORD ownerThreadId_ = 0;
    };

}    // namespace jnifx::detail
