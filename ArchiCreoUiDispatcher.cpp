#include "ArchiCreoUiDispatcher.hpp"

#include <deque>
#include <mutex>
#include <new>
#include <utility>

namespace jnifx::detail {

struct ArchiCreoUiDispatcher::State final {
    std::mutex mutex;
    std::deque<Task> tasks;
    bool accepting = true;
    bool executing = false;
    HWND helperWindow = nullptr;
    DWORD ownerThreadId = 0;
};

ArchiCreoUiDispatcher::~ArchiCreoUiDispatcher() {
    shutdown();
}

bool ArchiCreoUiDispatcher::attachToCurrentThread() noexcept {
    if (helperWindow_ != nullptr) {
        return ownerThreadId_ == GetCurrentThreadId();
    }

    const DWORD threadId = GetCurrentThreadId();
    if (threadId == 0) {
        return false;
    }

    state_ = std::make_shared<State>();
    state_->ownerThreadId = threadId;

    ownerThreadId_ = threadId;

    if (!createHelperWindow()) {
        state_.reset();
        ownerThreadId_ = 0;
        return false;
    }

    return true;
}

bool ArchiCreoUiDispatcher::createHelperWindow() noexcept {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &ArchiCreoUiDispatcher::wndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kWindowClassName;

    const ATOM atom = RegisterClassExW(&wc);
    if (atom == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    // The window owns a shared_ptr reference through this heap holder. This
    // keeps State alive until WM_NCDESTROY even if shutdown() runs elsewhere.
    auto* holder = new (std::nothrow) std::shared_ptr<State>(state_);
    if (holder == nullptr) {
        if (atom != 0) {
            UnregisterClassW(kWindowClassName, wc.hInstance);
        }
        return false;
    }

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
        holder);

    if (helperWindow_ == nullptr) {
        delete holder;
        if (atom != 0) {
            UnregisterClassW(kWindowClassName, wc.hInstance);
        }
        return false;
    }

    state_->helperWindow = helperWindow_;
    return true;
}

bool ArchiCreoUiDispatcher::post(Task task) noexcept {
    if (!task || helperWindow_ == nullptr || state_ == nullptr) {
        return false;
    }

    {
        std::lock_guard lock(state_->mutex);

        if (!state_->accepting || state_->helperWindow == nullptr) {
            return false;
        }

        // Bound the queue so a broken producer cannot grow memory without limit.
        constexpr std::size_t maxPendingTasks = 256;
        if (state_->tasks.size() >= maxPendingTasks) {
            return false;
        }

        try {
            state_->tasks.emplace_back(std::move(task));
        } catch (...) {
            return false;
        }
    }

    // PostMessage only transfers a wake-up signal. The task itself stays in the
    // protected C++ queue, avoiding arbitrary pointers in LPARAM/WPARAM.
    if (PostMessageW(helperWindow_, kExecuteMessage, 0, 0) == FALSE) {
        // Do not try to remove "the" task here: another producer may have
        // appended a task concurrently. Leaving the task in the shared queue
        // is safe; shutdown() clears it, and a previously queued wake-up may
        // still execute it. The caller is told that this particular wake-up
        // could not be guaranteed.
        return false;
    }

    return true;
}

void ArchiCreoUiDispatcher::shutdown() noexcept {
    const HWND helper = helperWindow_;
    const DWORD ownerThread = ownerThreadId_;
    const auto state = state_;

    if (helper == nullptr || state == nullptr) {
        helperWindow_ = nullptr;
        ownerThreadId_ = 0;
        state_.reset();
        return;
    }

    {
        std::lock_guard lock(state->mutex);
        state->accepting = false;
    }

    helperWindow_ = nullptr;
    ownerThreadId_ = 0;
    state_.reset();

    if (GetCurrentThreadId() == ownerThread) {
        auto* holder = reinterpret_cast<std::shared_ptr<State>*>(
            GetWindowLongPtrW(helper, GWLP_USERDATA));

        if (holder != nullptr && *holder != nullptr) {
            auto localState = *holder;

            // Do not execute queued tasks during teardown. They may refer to
            // plugin objects that are already being destroyed.
            {
                std::lock_guard lock(localState->mutex);
                localState->tasks.clear();
                localState->helperWindow = nullptr;
            }

            SetWindowLongPtrW(helper, GWLP_USERDATA, 0);
            delete holder;
        }

        DestroyWindow(helper);
        UnregisterClassW(kWindowClassName, GetModuleHandleW(nullptr));
        return;
    }

    DWORD_PTR ignored = 0;
    SendMessageTimeoutW(
        helper,
        kDestroyMessage,
        0,
        0,
        SMTO_ABORTIFHUNG | SMTO_BLOCK,
        2000,
        &ignored);
}

bool ArchiCreoUiDispatcher::isAttached() const noexcept {
    return helperWindow_ != nullptr &&
           ownerThreadId_ != 0 &&
           state_ != nullptr;
}

LRESULT CALLBACK ArchiCreoUiDispatcher::wndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {

    auto* holder = reinterpret_cast<std::shared_ptr<State>*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        const auto* create =
            reinterpret_cast<const CREATESTRUCTW*>(lParam);

        holder =
            static_cast<std::shared_ptr<State>*>(
                create->lpCreateParams);

        SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(holder));
    }

    if (holder == nullptr || *holder == nullptr) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    auto state = *holder;

    switch (message) {
    case kExecuteMessage: {
        for (;;) {
            Task task;

            {
                std::lock_guard lock(state->mutex);

                if (state->tasks.empty()) {
                    break;
                }

                task = std::move(state->tasks.front());
                state->tasks.pop_front();
                state->executing = true;
            }

            try {
                if (task) {
                    task();
                }
            } catch (...) {
                // A client task must never escape into the Windows message loop.
            }

            {
                std::lock_guard lock(state->mutex);
                state->executing = false;
            }
        }

        return 0;
    }

    case kDestroyMessage: {
        {
            std::lock_guard lock(state->mutex);
            state->accepting = false;
            state->tasks.clear();
            state->helperWindow = nullptr;
        }

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        delete holder;
        DestroyWindow(hwnd);
        UnregisterClassW(
            kWindowClassName,
            GetModuleHandleW(nullptr));

        return 0;
    }

    case WM_NCDESTROY:
        // If normal DestroyWindow() reaches here, the holder has not already
        // been consumed by kDestroyMessage.
        if (GetWindowLongPtrW(hwnd, GWLP_USERDATA) != 0) {
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            delete holder;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);

    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

} // namespace jnifx::detail
