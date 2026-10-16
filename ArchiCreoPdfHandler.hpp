#pragma once

#include "ArchiCreoCompat.hpp"
#include "ArchiCreoErrorHandler.hpp"
#include "ArchiCreoModelHandler.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace creo {

    class ArchiCreoPdfOptions final {
      public:
        ArchiCreoPdfOptions() {
            CREO_CHECK(
                detail::pdfOptionsAlloc(
                    &options_));

            if (options_ == nullptr) {
                throw std::runtime_error(
                    "ArchiCreoPdfOptions: Creo returned a null options handle");
            }
        }

        ArchiCreoPdfOptions(
            const ArchiCreoPdfOptions&) = delete;

        ArchiCreoPdfOptions& operator=(
            const ArchiCreoPdfOptions&) = delete;

        ArchiCreoPdfOptions(
            ArchiCreoPdfOptions&& other) noexcept :
            options_(other.options_) {
            other.options_ = nullptr;
        }

        ArchiCreoPdfOptions& operator=(
            ArchiCreoPdfOptions&& other) noexcept {
            if (this == &other) {
                return *this;
            }

            reset();
            options_ = other.options_;
            other.options_ = nullptr;
            return *this;
        }

        ~ArchiCreoPdfOptions() noexcept {
            reset();
        }

        [[nodiscard]] bool isValid() const noexcept {
            return options_ != nullptr;
        }

        [[nodiscard]] detail::RawPdfOptions raw() const noexcept {
            return options_;
        }

        void setInt(
            detail::RawPdfOptionType type,
            int value) {
            requireHandle();

            CREO_CHECK(
                detail::pdfOptionsIntPropertySet(
                    options_,
                    type,
                    value));
        }

        void setBool(
            detail::RawPdfOptionType type,
            bool value) {
            requireHandle();

            CREO_CHECK(
                detail::pdfOptionsBoolPropertySet(
                    options_,
                    type,
                    value
                        ? PRO_B_TRUE
                        : PRO_B_FALSE));
        }

        void setString(
            detail::RawPdfOptionType type,
            const std::wstring& value) {
            requireHandle();

            if (value.size() >=
                static_cast<std::size_t>(
                    PRO_PATH_SIZE)) {
                throw std::length_error(
                    "ArchiCreoPdfOptions::setString: "
                    "value is too long");
            }

            wchar_t buffer[PRO_PATH_SIZE] = {};

            std::copy(
                value.begin(),
                value.end(),
                buffer);

            CREO_CHECK(
                detail::pdfOptionsStringPropertySet(
                    options_,
                    type,
                    buffer));
        }

        void setNullableString(
            detail::RawPdfOptionType type,
            const wchar_t* value) {
            requireHandle();

            if (value == nullptr) {
                CREO_CHECK(
                    detail::pdfOptionsStringPropertySet(
                        options_,
                        type,
                        nullptr));
                return;
            }

            setString(
                type,
                std::wstring(value));
        }

        void setDouble(
            detail::RawPdfOptionType type,
            double value) {
            requireHandle();

            CREO_CHECK(
                detail::pdfOptionsDoublePropertySet(
                    options_,
                    type,
                    value));
        }

      private:
        void requireHandle() const {
            if (options_ == nullptr) {
                throw std::runtime_error(
                    "ArchiCreoPdfOptions: invalid options handle");
            }
        }

        void reset() noexcept {
            if (options_ == nullptr) {
                return;
            }

            (void)detail::pdfOptionsFree(
                options_);

            options_ = nullptr;
        }

        detail::RawPdfOptions options_ = nullptr;
    };

    class ArchiCreoPdfHandler final {
      public:
        ArchiCreoPdfHandler() = delete;

        static void exportModel(
            const ArchiCreoModelHandler& model,
            const std::filesystem::path& outputFile,
            const ArchiCreoPdfOptions& options) {

            requireExportableModel(
                model,
                "ArchiCreoPdfHandler::exportModel");

            const detail::RawPath output_path =
                makePdfPath(
                    outputFile,
                    "ArchiCreoPdfHandler::exportModel");

            if (!options.isValid()) {
                throw std::invalid_argument(
                    "ArchiCreoPdfHandler::exportModel: "
                    "invalid PDF options");
            }

            CREO_CHECK(
                detail::pdfExport(
                    model.raw(),
                    output_path,
                    options.raw()));
        }

        static void exportDrawing(
            const ArchiCreoModelHandler& drawing,
            const std::filesystem::path& outputFile) {

            if (!drawing.isValid()) {
                throw std::invalid_argument(
                    "ArchiCreoPdfHandler::exportDrawing: "
                    "invalid drawing model");
            }

            if (drawing.type() != PRO_MDL_DRAWING) {
                throw std::invalid_argument(
                    "ArchiCreoPdfHandler::exportDrawing: "
                    "model is not a drawing");
            }

            const detail::RawPath output_path =
                makePdfPath(
                    outputFile,
                    "ArchiCreoPdfHandler::exportDrawing");

            ArchiCreoPdfOptions options;
            CREO_CHECK(
                detail::pdfExport(
                    drawing.raw(),
                    output_path,
                    options.raw()));
        }

        static void exportDrawingWithProfile(
            const ArchiCreoModelHandler& drawing,
            const std::filesystem::path& outputFile,
            const std::filesystem::path& profile) {

            if (!drawing.isValid()) {
                throw std::invalid_argument(
                    "ArchiCreoPdfHandler::exportDrawingWithProfile: "
                    "invalid drawing model");
            }

            if (drawing.type() != PRO_MDL_DRAWING) {
                throw std::invalid_argument(
                    "ArchiCreoPdfHandler::exportDrawingWithProfile: "
                    "model is not a drawing");
            }

            const detail::RawPath output_path =
                makePdfPath(
                    outputFile,
                    "ArchiCreoPdfHandler::exportDrawingWithProfile");

            const detail::RawPath profile_path =
                makePath(
                    profile,
                    "ArchiCreoPdfHandler::exportDrawingWithProfile");

            CREO_CHECK(
                detail::pdfWithProfileExport(
                    drawing.raw(),
                    output_path,
                    profile_path));
        }

      private:
        static void requireExportableModel(
            const ArchiCreoModelHandler& model,
            const char* operation) {

            if (!model.isValid()) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": invalid model");
            }

            const auto model_type =
                model.type();

            if (model_type != PRO_MDL_DRAWING &&
                model_type != PRO_MDL_PART &&
                model_type != PRO_MDL_ASSEMBLY) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": unsupported model type for PDF export");
            }
        }

        static detail::RawPath makePdfPath(
            const std::filesystem::path& outputFile,
            const char* operation) {

            if (outputFile.empty()) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": empty output file");
            }

            const std::wstring value =
                outputFile.wstring();

            if (value.empty()) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": output file path is empty");
            }

            const std::wstring extension =
                outputFile.extension().wstring();

            if (!isPdfExtension(extension)) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": output file must have a .pdf extension");
            }

            return makePath(
                outputFile,
                operation);
        }

        static detail::RawPath makePath(
            const std::filesystem::path& file,
            const char* operation) {

            const std::wstring value =
                file.wstring();

            if (value.empty()) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": empty path");
            }

            if (value.size() >=
                static_cast<std::size_t>(
                    PRO_PATH_SIZE)) {
                throw std::length_error(
                    std::string(operation) +
                    ": path exceeds PRO_PATH_SIZE");
            }

            detail::RawPath result{};

            std::copy(
                value.begin(),
                value.end(),
                result);

            result[value.size()] = L'\0';
            return result;
        }

        static bool isPdfExtension(
            const std::wstring& extension) noexcept {

            if (extension.size() != 4) {
                return false;
            }

            return
                (extension[0] == L'.') &&
                (extension[1] == L'p' ||
                 extension[1] == L'P') &&
                (extension[2] == L'd' ||
                 extension[2] == L'D') &&
                (extension[3] == L'f' ||
                 extension[3] == L'F');
        }
    };

}
