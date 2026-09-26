#pragma once

#include <stdexcept>
#include <string>

#include "ArchiCreoCompat.hpp"
#include "ArchiCreoErrorHandler.hpp"
#include "ArchiScopeGuard.hpp"

namespace creo {

    class ArchiCreoWindowHandler final {
      public:
        ArchiCreoWindowHandler() noexcept :
            window_id_(PRO_VALUE_UNUSED) {
        }

        explicit ArchiCreoWindowHandler(int window_id) noexcept :
            window_id_(window_id) {
        }

        static ArchiCreoWindowHandler current() {
            int window_id = PRO_VALUE_UNUSED;
            CREO_CHECK(detail::windowCurrentGet(&window_id));
            return ArchiCreoWindowHandler(window_id);
        }

        static void refreshCurrent() {
            CREO_CHECK(detail::windowRefresh(PRO_VALUE_UNUSED));
        }

        static void repaintCurrent() {
            CREO_CHECK(detail::windowRepaint(PRO_VALUE_UNUSED));
        }

        [[nodiscard]] int id() const noexcept {
            return window_id_;
        }

        [[nodiscard]] bool isValid() const noexcept {
            return window_id_ != PRO_VALUE_UNUSED;
        }

        void refresh() const {
            CREO_CHECK(detail::windowRefresh(window_id_));
        }

        void repaint() const {
            CREO_CHECK(detail::windowRepaint(window_id_));
        }

        void activate() const {
            requireWindow();
            CREO_CHECK(detail::windowActivate(window_id_));
        }

        void setCurrent() const {
            requireWindow();
            CREO_CHECK(detail::windowCurrentSet(window_id_));
        }

        detail::RawMdl model() const {
            requireWindow();
            detail::RawMdl model = nullptr;
            CREO_CHECK(
                detail::windowMdlGet(window_id_, &model));
            return model;
        }

        std::string name() const {
            requireWindow();

            char* raw_name = nullptr;
            CREO_CHECK(
                detail::windowNameGet(window_id_, &raw_name));

            auto free_raw_name = Defer([&] {
                if (raw_name != nullptr) {
                    (void)detail::stringFree(raw_name);
                }
            });

            return raw_name == nullptr
                ? std::string{}
                : std::string(raw_name);
        }

      private:
        void requireWindow() const {
            if (window_id_ == PRO_VALUE_UNUSED) {
                throw std::invalid_argument(
                    "ArchiCreoWindowHandler: invalid window id");
            }
        }

        int window_id_;
    };

}
