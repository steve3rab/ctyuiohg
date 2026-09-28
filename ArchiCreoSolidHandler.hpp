#pragma once

#include <stdexcept>

#include "ArchiCreoCompat.hpp"
#include "ArchiCreoErrorHandler.hpp"

namespace creo {

    class ArchiCreoSolidHandler final {
      public:
        ArchiCreoSolidHandler() noexcept :
            handle_(nullptr) {
        }

        explicit ArchiCreoSolidHandler(
            detail::RawSolid handle) noexcept :
            handle_(handle) {
        }

        [[nodiscard]] detail::RawSolid raw() const noexcept {
            return handle_;
        }

        [[nodiscard]] bool isValid() const noexcept {
            return handle_ != nullptr;
        }

        detail::ProErrorCode regenerate(
            int flags = PRO_REGEN_NO_FLAGS) const {
            requireHandle();

            if (flags < 0) {
                throw std::invalid_argument(
                    "ArchiCreoSolidHandler::regenerate: negative flags");
            }

            validateFlagCombinations(flags);

            detail::ProErrorCode status =
                detail::solidRegenerate(handle_, flags);

            if (status == PRO_TK_REGEN_AGAIN) {
                status =
                    detail::solidRegenerate(handle_, flags);
            }

            if (status == PRO_TK_UNATTACHED_FEATS) {
                return status;
            }

            throwIfError(
                status,
                "ProSolidRegenerate");

            return status;
        }

      private:
        static void validateFlagCombinations(int flags) {
            if ((flags & PRO_REGEN_UPDATE_ASSEMBLY_ONLY) != 0 &&
                (flags & PRO_REGEN_FORCE_REGEN) != 0) {
                throw std::invalid_argument(
                    "ArchiCreoSolidHandler::regenerate: "
                    "PRO_REGEN_UPDATE_ASSEMBLY_ONLY cannot be "
                    "combined with PRO_REGEN_FORCE_REGEN");
            }

            if ((flags & PRO_REGEN_TOP_ASM_ONLY) != 0 &&
                (flags & PRO_REGEN_FORCE_REGEN) != 0) {
                throw std::invalid_argument(
                    "ArchiCreoSolidHandler::regenerate: "
                    "PRO_REGEN_TOP_ASM_ONLY cannot be "
                    "combined with PRO_REGEN_FORCE_REGEN");
            }

            if ((flags & PRO_REGEN_ALLOW_CONFIRM) != 0 &&
                (flags & PRO_REGEN_UNDO_IF_FAIL) != 0) {
                throw std::invalid_argument(
                    "ArchiCreoSolidHandler::regenerate: "
                    "PRO_REGEN_ALLOW_CONFIRM cannot be combined "
                    "with PRO_REGEN_UNDO_IF_FAIL");
            }

            if ((flags & PRO_REGEN_SKIP_DISALLOW_SYS_RECOVER) != 0 &&
                (flags & PRO_REGEN_CAN_FIX) == 0) {
                throw std::invalid_argument(
                    "ArchiCreoSolidHandler::regenerate: "
                    "PRO_REGEN_SKIP_DISALLOW_SYS_RECOVER requires "
                    "PRO_REGEN_CAN_FIX");
            }

            if ((flags & PRO_REGEN_RGN_BCK_USING_DISK) != 0 &&
                (flags & PRO_REGEN_CAN_FIX) == 0) {
                throw std::invalid_argument(
                    "ArchiCreoSolidHandler::regenerate: "
                    "PRO_REGEN_RGN_BCK_USING_DISK requires "
                    "PRO_REGEN_CAN_FIX");
            }

            if ((flags & PRO_REGEN_NO_RESOLVE_MODE) != 0 &&
                (flags & PRO_REGEN_RESOLVE_MODE) != 0) {
                throw std::invalid_argument(
                    "ArchiCreoSolidHandler::regenerate: "
                    "PRO_REGEN_NO_RESOLVE_MODE cannot be combined "
                    "with PRO_REGEN_RESOLVE_MODE");
            }
        }

        void requireHandle() const {
            if (handle_ == nullptr) {
                throw std::invalid_argument(
                    "ArchiCreoSolidHandler: invalid (null) handle");
            }
        }

        detail::RawSolid handle_;
    };

}
