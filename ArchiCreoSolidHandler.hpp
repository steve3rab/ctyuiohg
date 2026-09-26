#pragma once

#include <stdexcept>

#include "ArchiCreoCompat.hpp"
#include "ArchiCreoErrorHandler.hpp"

namespace creo {

    class ArchiCreoSolidHandler {
      public:
        ArchiCreoSolidHandler() noexcept : handle_(nullptr) {
        }
        explicit ArchiCreoSolidHandler(detail::RawSolid handle) noexcept : handle_(handle) {
        }

        static void checkRegenFlags(int flags) {
            if (flags < 0 || flags > (PRO_REGEN_LAST_USED * 2 - 1)) {
                throw std::invalid_argument("ArchiCreoSolidHandler::regenerate: unknown PRO_REGEN_* bits");
            }
            if ((flags & PRO_REGEN_UPDATE_ASSEMBLY_ONLY) != 0 &&
                (flags & PRO_REGEN_FORCE_REGEN) != 0) {
                throw std::invalid_argument(
                    "ArchiCreoSolidHandler::regenerate: PRO_REGEN_UPDATE_ASSEMBLY_ONLY cannot be combined with "
                    "PRO_REGEN_FORCE_REGEN");
            }
            if ((flags & PRO_REGEN_ALLOW_CONFIRM) != 0 &&
                (flags & PRO_REGEN_UNDO_IF_FAIL) != 0) {
                throw std::invalid_argument(
                    "ArchiCreoSolidHandler::regenerate: PRO_REGEN_ALLOW_CONFIRM and "
                    "PRO_REGEN_UNDO_IF_FAIL cannot be used together");
            }
        }

        detail::RawSolid raw() const noexcept {
            return handle_;
        }

        bool isValid() const noexcept {
            return handle_ != nullptr;
        }

        detail::ProErrorCode regenerate(int flags = PRO_REGEN_NO_FLAGS) const {
            requireHandle();
            checkRegenFlags(flags);

            detail::ProErrorCode status = detail::solidRegenerate(handle_, flags);

            if (status == PRO_TK_REGEN_AGAIN) {
                status = detail::solidRegenerate(handle_, flags);
            }

            if (status == PRO_TK_UNATTACHED_FEATS) {
                return status;
            }

            throwIfError(status, "ProSolidRegenerate");
            return status;
        }

      private:
        void requireHandle() const {
            if (handle_ == nullptr) {
                throw std::invalid_argument(
                    "ArchiCreoSolidHandler: invalid (null) handle");
            }
        }

        detail::RawSolid handle_;
    };

}
