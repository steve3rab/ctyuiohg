#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>

#include "ArchiCreoCompat.hpp"
#include "ArchiCreoErrorHandler.hpp"
#include "ArchiCreoSolidHandler.hpp"
#include "ArchiCreoWindowHandler.hpp"
#include "ArchiScopeGuard.hpp"

namespace creo {

    struct ModelInfo {
        std::wstring name;
        detail::RawMdlType type;
        std::wstring path;
        std::wstring extension;
    };

    class ArchiCreoModelHandler {
      public:
        ArchiCreoModelHandler() noexcept : handle_(nullptr) {
        }
        explicit ArchiCreoModelHandler(detail::RawMdl handle) noexcept : handle_(handle) {
        }

        static ArchiCreoModelHandler fromCurrentWindow() {
            detail::RawMdl model = nullptr;
            const detail::ProErrorCode current_error =
                detail::mdlCurrentGet(&model);

            if (current_error != detail::kNoError &&
                current_error != PRO_TK_BAD_CONTEXT) {
                CREO_CHECK(current_error);
            }

            if (model == nullptr) {
                ArchiCreoWindowHandler current = ArchiCreoWindowHandler::current();
                if (!current.isValid()) {
                    throw std::runtime_error("ArchiCreoModelHandler: the current window is the message area");
                }
                model = current.model();
            }
            if (model == nullptr) {
                throw std::runtime_error("ArchiCreoModelHandler: no model in the current window");
            }
            return ArchiCreoModelHandler(model);
        }

        static bool isDisplayableType(detail::RawMdlType type) noexcept {
            switch (type) {
                case PRO_MDL_ASSEMBLY:
                case PRO_MDL_PART:
                case PRO_MDL_2DSECTION:
                case PRO_MDL_DRAWING:
                case PRO_MDL_LAYOUT:
                case PRO_MDL_DWGFORM:
                case PRO_MDL_MFG:
                case PRO_MDL_REPORT:
                case PRO_MDL_MARKUP:
                case PRO_MDL_DIAGRAM:
                    return true;
                default:
                    return false;
            }
        }

        static bool isSolidType(detail::RawMdlType type) noexcept {
            return type == PRO_MDL_PART || type == PRO_MDL_ASSEMBLY;
        }

        static ArchiCreoModelHandler createPart(const std::wstring& model_name) {
            if (model_name.empty()) {
                throw std::invalid_argument(
                    "ArchiCreoModelHandler::createPart: empty model name");
            }

            constexpr std::size_t kCreateNameMaxLength = 31;
            if (model_name.size() > kCreateNameMaxLength) {
                throw std::invalid_argument(
                    "ArchiCreoModelHandler::createPart: model name too long");
            }

            wchar_t name[PRO_MDLNAME_SIZE] = {};
            std::copy(model_name.begin(), model_name.end(), name);

            detail::RawSolid solid = nullptr;
            CREO_CHECK(detail::solidMdlnameCreate(
                name,
                PRO_MDLFILE_PART,
                &solid));

            if (solid == nullptr) {
                throw std::runtime_error(
                    "ArchiCreoModelHandler::createPart: Creo returned a null handle");
            }

            return ArchiCreoModelHandler(static_cast<detail::RawMdl>(solid));
        }

        detail::RawMdl raw() const noexcept {
            return handle_;
        }

        bool isValid() const noexcept {
            return handle_ != nullptr;
        }

        std::wstring name() const {
            requireHandle();
            wchar_t buffer[PRO_MDLNAME_SIZE] = {};
            CREO_CHECK(detail::mdlMdlNameGet(handle_, buffer));
            return std::wstring(buffer);
        }

        detail::RawMdlType type() const {
            requireHandle();
            detail::RawMdlType type = PRO_MDL_UNUSED;
            CREO_CHECK(detail::mdlTypeGet(handle_, &type));
            return type;
        }

        std::wstring directoryPath() const {
            requireHandle();
            wchar_t buffer[PRO_PATH_SIZE] = {};
            CREO_CHECK(detail::mdlDirectoryPathGet(handle_, buffer));
            return std::wstring(buffer);
        }

        std::wstring extension() const {
            requireHandle();
            wchar_t buffer[PRO_MDLEXTENSION_SIZE] = {};
            CREO_CHECK(detail::mdlExtensionGet(handle_, buffer));
            return std::wstring(buffer);
        }

        ArchiCreoWindowHandler window() const {
            requireHandle();
            int window_id = PRO_VALUE_UNUSED;
            CREO_CHECK(detail::mdlWindowGet(handle_, &window_id));
            return ArchiCreoWindowHandler(window_id);
        }

        bool isModified() const {
            requireHandle();
            detail::RawBoolean modified = PRO_B_FALSE;
            CREO_CHECK(detail::mdlModificationVerify(handle_, &modified));
            return modified == PRO_B_TRUE;
        }

        ModelInfo info() const {
            requireHandle();
            ModelInfo info{};
            info.name = name();
            info.type = type();
            info.path = directoryPath();
            info.extension = extension();
            return info;
        }

        void display() const {
            requireHandle();
            if (!isDisplayableType(type())) {
                throw std::runtime_error("ArchiCreoModelHandler::display: this model type cannot be displayed");
            }
            CREO_CHECK(detail::mdlDisplay(handle_));
        }

        ArchiCreoWindowHandler displayInNewWindow() const {
            requireHandle();
            const detail::RawMdlType model_type = this->type();
            if (!isDisplayableType(model_type)) {
                throw std::runtime_error("ArchiCreoModelHandler::displayInNewWindow: this model type cannot be displayed");
            }

            wchar_t object_name[PRO_MDLNAME_SIZE] = {};
            CREO_CHECK(detail::mdlMdlNameGet(handle_, object_name));

            ArchiCreoWindowHandler previous_window = ArchiCreoWindowHandler::current();

            int window_id = PRO_VALUE_UNUSED;
            CREO_CHECK(detail::objectwindowMdlnameCreate(
                object_name,
                static_cast<detail::RawObjectType>(model_type),
                &window_id));
            ArchiCreoWindowHandler window(window_id);

            window.setCurrent();

            auto restore_current_window = Defer([&] {
                if (previous_window.isValid()) {
                    detail::windowCurrentSet(previous_window.id());
                }
            });

            CREO_CHECK(detail::mdlDisplay(handle_));
            window.activate();

            restore_current_window.Dismiss();
            return window;
        }

        void save() const {
            requireHandle();
            if (isDisplayableType(type())) {
                CREO_CHECK(detail::mdlDisplay(handle_));
            }
            CREO_CHECK(detail::mdlSave(handle_));
            if (isModified()) {
                throw std::runtime_error(
                    "ArchiCreoModelHandler::save: model still reports unsaved modifications");
            }
        }

        ArchiCreoSolidHandler asSolid() const {
            requireHandle();
            if (!isSolidType(type())) {
                throw std::runtime_error(
                    "ArchiCreoModelHandler::asSolid: only a part or an assembly is a solid");
            }
            return ArchiCreoSolidHandler(detail::mdlToSolid(handle_));
        }

        detail::ProErrorCode regenerate(int flags = PRO_REGEN_NO_FLAGS) const {
            return asSolid().regenerate(flags);
        }

        void rename(const std::wstring& new_name) const {
            requireHandle();
            if (new_name.empty()) {
                throw std::invalid_argument(
                    "ArchiCreoModelHandler::rename: empty name");
            }
            if (new_name.size() >= static_cast<std::size_t>(PRO_MDLNAME_SIZE)) {
                throw std::invalid_argument(
                    "ArchiCreoModelHandler::rename: name too long");
            }

            wchar_t mutable_name[PRO_MDLNAME_SIZE] = {};
            std::copy(new_name.begin(), new_name.end(), mutable_name);
            CREO_CHECK(detail::mdlnameRename(handle_, mutable_name));
        }

      private:
        void requireHandle() const {
            if (handle_ == nullptr) {
                throw std::invalid_argument(
                    "ArchiCreoModelHandler: invalid (null) handle");
            }
        }

        detail::RawMdl handle_;
    };

}
