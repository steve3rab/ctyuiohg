#pragma once

#include "ArchiCreoCompat.hpp"
#include "ArchiCreoErrorHandler.hpp"
#include "ArchiCreoModelHandler.hpp"
#include "ArchiScopeGuard.hpp"

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <stdexcept>
#include <string>

namespace creo {

    class ArchiCreoImportHandler final {
      public:
        ArchiCreoImportHandler() = delete;

        static ArchiCreoModelHandler importStep(
            const std::filesystem::path& file,
            const std::wstring& modelName) {

            validateRegularFile(file, L".stp", L".step");
            validateModelName(modelName);

            detail::RawPath importPath{};
            copyPath(
                file,
                importPath,
                "ArchiCreoImportHandler::importStep");

            detail::RawMdlName name{};
            copyModelName(
                modelName,
                name,
                "ArchiCreoImportHandler::importStep");

            detail::RawMdl importedModel = nullptr;

            CREO_CHECK(
                detail::intfImportModelWithOptionsMdlnameCreate(
                    importPath,
                    nullptr,
                    PRO_INTF_IMPORT_STEP,
                    PRO_MDL_UNUSED,
                    PRO_IMPORTREP_MASTER,
                    name,
                    nullptr,
                    nullptr,
                    &importedModel));

            if (importedModel == nullptr) {
                throw std::runtime_error(
                    "ArchiCreoImportHandler::importStep: "
                    "Creo returned a null model handle");
            }

            auto eraseOnFailure = Defer([&] {
                if (importedModel != nullptr) {
                    (void)detail::mdlErase(importedModel);
                }
            });

            ArchiCreoModelHandler result(importedModel);

            if (!result.isValid()) {
                throw std::runtime_error(
                    "ArchiCreoImportHandler::importStep: "
                    "invalid imported model handle");
            }

            eraseOnFailure.Dismiss();
            return result;
        }

        static ArchiCreoModelHandler importEmn(
            const std::filesystem::path& emnFile,
            const std::filesystem::path& empFile = {}) {

            validateRegularFile(
                emnFile,
                L".emn");

            if (!empFile.empty()) {
                validateRegularFile(
                    empFile,
                    L".emp");
            }

            detail::RawPath importPath{};
            copyPath(
                emnFile,
                importPath,
                "ArchiCreoImportHandler::importEmn");

            detail::RawEcadImpOpts options = nullptr;

            CREO_CHECK(
                detail::ecadImpOptsAlloc(
                    &options));

            auto freeOptions = Defer([&] {
                (void)detail::ecadImpOptsFree(
                    &options);
            });

            if (!empFile.empty()) {
                detail::RawPath empPath{};
                copyPath(
                    empFile,
                    empPath,
                    "ArchiCreoImportHandler::importEmn");

                CREO_CHECK(
                    detail::ecadImpOptsIDF3EmpFileNameSet(
                        options,
                        empPath));
            }

            detail::RawMdl importedModel = nullptr;

            CREO_CHECK(
                detail::ecadImport(
                    importPath,
                    options,
                    PRO_TK_ECAD_IDF3,
                    &importedModel));

            if (importedModel == nullptr) {
                throw std::runtime_error(
                    "ArchiCreoImportHandler::importEmn: "
                    "Creo returned a null assembly handle");
            }

            auto eraseOnFailure = Defer([&] {
                if (importedModel != nullptr) {
                    (void)detail::mdlErase(importedModel);
                }
            });

            ArchiCreoModelHandler result(importedModel);

            if (!result.isAssembly()) {
                throw std::runtime_error(
                    "ArchiCreoImportHandler::importEmn: "
                    "Creo did not return an assembly");
            }

            eraseOnFailure.Dismiss();
            return result;
        }

      private:
        static void validateRegularFile(
            const std::filesystem::path& file,
            std::initializer_list<const wchar_t*> extensions) {

            if (file.empty()) {
                throw std::invalid_argument(
                    "ArchiCreoImportHandler: empty file path");
            }

            std::error_code error;
            const bool exists =
                std::filesystem::exists(file, error);

            if (error || !exists ||
                !std::filesystem::is_regular_file(file, error) ||
                error) {
                throw std::invalid_argument(
                    "ArchiCreoImportHandler: file does not exist or "
                    "is not a regular file: " +
                    file.u8string());
            }

            const std::wstring extension =
                toLower(file.extension().wstring());

            bool supported = false;
            for (const wchar_t* expected : extensions) {
                if (extension == expected) {
                    supported = true;
                    break;
                }
            }

            if (!supported) {
                throw std::invalid_argument(
                    "ArchiCreoImportHandler: unsupported file extension: " +
                    file.u8string());
            }
        }

        static std::wstring toLower(
            const std::wstring& value) {

            std::wstring result = value;

            for (wchar_t& character : result) {
                if (character >= L'A' &&
                    character <= L'Z') {
                    character =
                        static_cast<wchar_t>(
                            character - L'A' + L'a');
                }
            }

            return result;
        }

        static void validateModelName(
            const std::wstring& modelName) {

            if (modelName.empty()) {
                throw std::invalid_argument(
                    "ArchiCreoImportHandler: empty model name");
            }

            constexpr std::size_t maxModelNameLength = 31;

            if (modelName.size() > maxModelNameLength) {
                throw std::invalid_argument(
                    "ArchiCreoImportHandler: "
                    "model name exceeds 31 characters");
            }
        }

        static void copyPath(
            const std::filesystem::path& path,
            detail::RawPath destination,
            const char* operation) {

            const std::wstring value = path.wstring();

            if (value.empty()) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": empty file path");
            }

            if (value.size() >=
                static_cast<std::size_t>(PRO_PATH_SIZE)) {
                throw std::length_error(
                    std::string(operation) +
                    ": file path exceeds Creo PRO_PATH_SIZE");
            }

            std::copy(
                value.begin(),
                value.end(),
                destination);

            destination[value.size()] = L'\0';
        }

        static void copyModelName(
            const std::wstring& modelName,
            detail::RawMdlName destination,
            const char* operation) {

            if (modelName.size() >=
                static_cast<std::size_t>(PRO_NAME_SIZE)) {
                throw std::length_error(
                    std::string(operation) +
                    ": model name exceeds Creo PRO_NAME_SIZE");
            }

            std::copy(
                modelName.begin(),
                modelName.end(),
                destination);

            destination[modelName.size()] = L'\0';
        }
    };

}
