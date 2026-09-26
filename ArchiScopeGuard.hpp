#pragma once

#include <type_traits>
#include <utility>

namespace creo {

    template<typename F>
    class ArchiScopeGuard final {
      public:
        explicit ArchiScopeGuard(F f) noexcept(std::is_nothrow_move_constructible_v<F>) :
            f_(std::move(f)) {
        }

        ArchiScopeGuard(ArchiScopeGuard&& other) noexcept(std::is_nothrow_move_constructible_v<F>) :
            f_(std::move(other.f_)),
            active_(other.active_) {
            other.active_ = false;
        }

        ArchiScopeGuard(const ArchiScopeGuard&) = delete;
        ArchiScopeGuard& operator=(const ArchiScopeGuard&) = delete;
        ArchiScopeGuard& operator=(ArchiScopeGuard&&) = delete;

        ~ArchiScopeGuard() noexcept {
            if (active_) {
                f_();
            }
        }

        void Dismiss() noexcept {
            active_ = false;
        }

        [[nodiscard]] bool active() const noexcept {
            return active_;
        }

      private:
        F f_;
        bool active_ = true;
    };

    template<typename F>
    [[nodiscard]] ArchiScopeGuard<std::decay_t<F>> Defer(F&& f) {
        return ArchiScopeGuard<std::decay_t<F>>(std::forward<F>(f));
    }

}
