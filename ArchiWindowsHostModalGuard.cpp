#include "ArchiWindowsHostModalGuard.hpp"

#include <algorithm>
#include <stdexcept>

namespace jnifx::detail {

    ArchiWindowsHostModalGuard::ArchiWindowsHostModalGuard(HWND hostWindow) {
        if (!attach(hostWindow)) {
            throw std::runtime_error("Unable to attach the Creo host modal guard");
        }
    }

    ArchiWindowsHostModalGuard::~ArchiWindowsHostModalGuard() {
        detach();
    }

    bool ArchiWindowsHostModalGuard::attach(HWND hostWindow) {
        if (helperWindow_ != nullptr) {
            return hostWindow_ == hostWindow;
        }
        if (hostWindow == nullptr || !IsWindow(hostWindow)) {
            return false;
        }

        DWORD hostProcessId = 0;
        const DWORD hostThreadId = GetWindowThreadProcessId(hostWindow, &hostProcessId);
        if (hostThreadId == 0 || hostProcessId != GetCurrentProcessId() || hostThreadId != GetCurrentThreadId()) {
            return false;
        }

        hostWindow_ = hostWindow;
        ownerThreadId_ = hostThreadId;
        if (!createHelperWindow()) {
            hostWindow_ = nullptr;
            ownerThreadId_ = 0;
            return false;
        }
        return true;
    }

    bool ArchiWindowsHostModalGuard::attachBestEffortFromCurrentThread() noexcept {
        if (helperWindow_ != nullptr) {
            return true;
        }

        if (GetCurrentThreadId() == 0) {
            return false;
        }

        HWND candidate = GetActiveWindow();
        if (candidate == nullptr) {
            candidate = GetFocus();
        }
        if (candidate == nullptr) {
            candidate = GetForegroundWindow();
        }
        if (candidate == nullptr) {
            return false;
        }

        DWORD processId = 0;
        const DWORD threadId = GetWindowThreadProcessId(candidate, &processId);
        if (threadId == 0 || threadId != GetCurrentThreadId() || processId != GetCurrentProcessId()) {
            return false;
        }

        HWND root = GetAncestor(candidate, GA_ROOTOWNER);
        if (root == nullptr) {
            root = candidate;
        }
        return attach(root);
    }

    void ArchiWindowsHostModalGuard::detach() noexcept {
        const HWND helper = helperWindow_;
        const DWORD ownerThreadId = ownerThreadId_;
        if (helper == nullptr) {
            hostWindow_ = nullptr;
            ownerThreadId_ = 0;
            return;
        }

        const bool onOwnerThread = GetCurrentThreadId() == ownerThreadId;

        helperWindow_ = nullptr;
        hostWindow_ = nullptr;
        ownerThreadId_ = 0;

        if (onOwnerThread) {
            auto* state = reinterpret_cast<State*>(GetWindowLongPtrW(helper, GWLP_USERDATA));
            if (state != nullptr) {
                applyUnblockOnOwnerThread(*state);
            }
            DestroyWindow(helper);
            UnregisterClassW(kWindowClassName, GetModuleHandleW(nullptr));
            return;
        }

        // State is owned by the helper window, not by the C++ guard. Therefore a
        // foreign-thread destructor can synchronously ask the owner thread to
        // unblock and destroy the helper without leaving a callback to this object.
        DWORD_PTR ignored = 0;
        SendMessageTimeoutW(helper, kUnblockMessage, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 2000, &ignored);
        SendMessageTimeoutW(helper, kDestroyMessage, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 2000, &ignored);
    }

    bool ArchiWindowsHostModalGuard::createHelperWindow() {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = &ArchiWindowsHostModalGuard::wndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = kWindowClassName;

        const ATOM atom = RegisterClassExW(&wc);
        if (atom == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }

        auto* state = new (std::nothrow) State{};
        if (state == nullptr) {
            return false;
        }
        state->hostWindow = hostWindow_;
        state->ownerThreadId = ownerThreadId_;

        helperWindow_ = CreateWindowExW(
            0,
            kWindowClassName,
            kWindowClassName,
            0,
            0,
            0,
            0,
            0,
            HWND_MESSAGE,
            nullptr,
            wc.hInstance,
            state);

        if (helperWindow_ == nullptr) {
            delete state;
            UnregisterClassW(kWindowClassName, wc.hInstance);
            return false;
        }
        return true;
    }

    bool ArchiWindowsHostModalGuard::block(std::chrono::milliseconds timeout) {
        if (hostWindow_ == nullptr || helperWindow_ == nullptr || !IsWindow(hostWindow_)) {
            return false;
        }

        if (GetCurrentThreadId() == ownerThreadId_) {
            auto* state = reinterpret_cast<State*>(GetWindowLongPtrW(helperWindow_, GWLP_USERDATA));
            return state != nullptr && applyBlockOnOwnerThread(*state);
        }

        const auto boundedTimeout = std::max<std::chrono::milliseconds>(timeout, std::chrono::milliseconds(1));
        const auto timeoutLimit = std::min<std::chrono::milliseconds>(boundedTimeout, std::chrono::milliseconds(60000));
        const DWORD timeoutMs = static_cast<DWORD>(timeoutLimit.count());

        DWORD_PTR result = 0;
        const LRESULT sent = SendMessageTimeoutW(
            helperWindow_,
            kBlockMessage,
            0,
            0,
            SMTO_ABORTIFHUNG | SMTO_BLOCK,
            timeoutMs,
            &result);
        return sent != 0 && result != 0;
    }

    bool ArchiWindowsHostModalGuard::unblock(std::chrono::milliseconds timeout) noexcept {
        const HWND helper = helperWindow_;
        if (helper == nullptr) {
            return true;
        }

        if (GetCurrentThreadId() == ownerThreadId_) {
            auto* state = reinterpret_cast<State*>(GetWindowLongPtrW(helper, GWLP_USERDATA));
            if (state == nullptr) {
                return false;
            }
            applyUnblockOnOwnerThread(*state);
            return true;
        }

        const auto boundedTimeout = std::max<std::chrono::milliseconds>(timeout, std::chrono::milliseconds(1));
        const auto timeoutLimit = std::min<std::chrono::milliseconds>(boundedTimeout, std::chrono::milliseconds(60000));
        const DWORD timeoutMs = static_cast<DWORD>(timeoutLimit.count());

        DWORD_PTR result = 0;
        const LRESULT sent = SendMessageTimeoutW(
            helper,
            kUnblockMessage,
            0,
            0,
            SMTO_ABORTIFHUNG | SMTO_BLOCK,
            timeoutMs,
            &result);

        return sent != 0 && result != 0;
    }

    void ArchiWindowsHostModalGuard::unblockAsync() noexcept {
        const HWND helper = helperWindow_;
        if (helper == nullptr) {
            return;
        }

        if (GetCurrentThreadId() == ownerThreadId_) {
            auto* state = reinterpret_cast<State*>(GetWindowLongPtrW(helper, GWLP_USERDATA));
            if (state != nullptr) {
                applyUnblockOnOwnerThread(*state);
            }
            return;
        }

        PostMessageW(helper, kUnblockMessage, 0, 0);
    }

    void ArchiWindowsHostModalGuard::forceUnblockOnOwnerThread() noexcept {
        if (helperWindow_ == nullptr || GetCurrentThreadId() != ownerThreadId_) {
            return;
        }
        auto* state = reinterpret_cast<State*>(GetWindowLongPtrW(helperWindow_, GWLP_USERDATA));
        if (state != nullptr) {
            applyUnblockOnOwnerThread(*state);
        }
    }

    bool ArchiWindowsHostModalGuard::belongsToHostWindow(const State& state, HWND hwnd) noexcept {
        if (hwnd == nullptr) {
            return false;
        }
        if (hwnd == state.hostWindow) {
            return true;
        }
        if (GetWindowThreadProcessId(hwnd, nullptr) != state.ownerThreadId) {
            return false;
        }
        return GetAncestor(hwnd, GA_ROOTOWNER) == state.hostWindow;
    }

    BOOL CALLBACK ArchiWindowsHostModalGuard::enumTopLevelWindow(HWND hwnd, LPARAM lParam) {
        auto* state = reinterpret_cast<State*>(lParam);
        if (state == nullptr || !belongsToHostWindow(*state, hwnd) || !IsWindow(hwnd)) {
            return TRUE;
        }

        DisabledWindow snapshot;
        snapshot.hwnd = hwnd;
        snapshot.wasEnabled = IsWindowEnabled(hwnd) != FALSE;
        state->disabledWindows.push_back(snapshot);
        EnableWindow(hwnd, FALSE);
        return TRUE;
    }

    bool ArchiWindowsHostModalGuard::applyBlockOnOwnerThread(State& state) noexcept {
        if (state.hostWindow == nullptr || !IsWindow(state.hostWindow)) {
            return false;
        }
        if (state.blocked) {
            return true;
        }

        state.disabledWindows.clear();
        EnumThreadWindows(state.ownerThreadId, &ArchiWindowsHostModalGuard::enumTopLevelWindow,
            reinterpret_cast<LPARAM>(&state));

        if (state.disabledWindows.empty()) {
            return false;
        }

        state.blocked = true;
        return true;
    }

    void ArchiWindowsHostModalGuard::applyUnblockOnOwnerThread(State& state) noexcept {
        if (!state.blocked) {
            return;
        }

        for (auto it = state.disabledWindows.rbegin(); it != state.disabledWindows.rend(); ++it) {
            if (it->hwnd != nullptr && IsWindow(it->hwnd)) {
                EnableWindow(it->hwnd, it->wasEnabled ? TRUE : FALSE);
            }
        }
        state.disabledWindows.clear();
        state.blocked = false;
    }

    LRESULT CALLBACK ArchiWindowsHostModalGuard::wndProc(
        HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        auto* state = reinterpret_cast<State*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            state = static_cast<State*>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        }

        if (state == nullptr) {
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        switch (message) {
        case kBlockMessage:
            return applyBlockOnOwnerThread(*state) ? 1 : 0;
        case kUnblockMessage:
            applyUnblockOnOwnerThread(*state);
            return 1;
        case kDestroyMessage:
            applyUnblockOnOwnerThread(*state);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            delete state;
            DestroyWindow(hwnd);
            UnregisterClassW(kWindowClassName, GetModuleHandleW(nullptr));
            return 1;
        case WM_NCDESTROY:
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            delete state;
            return DefWindowProcW(hwnd, message, wParam, lParam);
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }

}    // namespace jnifx::detail
