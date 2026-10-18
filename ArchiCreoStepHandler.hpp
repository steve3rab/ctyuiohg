#pragma once

#include "ArchiCreoCompat.hpp"
#include "ArchiCreoErrorHandler.hpp"
#include "ArchiCreoModelHandler.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace creo {

    class ArchiCreoStepHandler final {
      public:
        ArchiCreoStepHandler() = delete;

        static void exportModel(
            const ArchiCreoModelHandler& model,
            const std::filesystem::path& outputFile) {

            if (!model.isValid()) {
                throw std::invalid_argument(
                    "ArchiCreoStepHandler::exportModel: "
                    "invalid model");
            }

            const detail::RawMdlType model_type =
                model.type();

            if (model_type != PRO_MDL_PART &&
                model_type != PRO_MDL_ASSEMBLY) {
                throw std::invalid_argument(
                    "ArchiCreoStepHandler::exportModel: "
                    "model must be a part or assembly");
            }

            const std::filesystem::path output_base =
                normalizeOutputBase(
                    outputFile,
                    "ArchiCreoStepHandler::exportModel");

            detail::RawPath output_path{};
            copyPath(
                output_base,
                output_path,
                "ArchiCreoStepHandler::exportModel");

            const detail::RawSolid solid =
                model.asSolid().raw();

            CREO_CHECK(
                detail::intf3DFileWriteWithDefaultProfile(
                    solid,
                    PRO_INTF_EXPORT_STEP,
                    output_path));
        }

        static void exportPart(
            const ArchiCreoModelHandler& part,
            const std::filesystem::path& outputFile) {

            requireType(
                part,
                PRO_MDL_PART,
                "ArchiCreoStepHandler::exportPart");

            exportModel(
                part,
                outputFile);
        }

        static void exportAssembly(
            const ArchiCreoModelHandler& assembly,
            const std::filesystem::path& outputFile) {

            requireType(
                assembly,
                PRO_MDL_ASSEMBLY,
                "ArchiCreoStepHandler::exportAssembly");

            exportModel(
                assembly,
                outputFile);
        }

      private:
        static void requireType(
            const ArchiCreoModelHandler& model,
            detail::RawMdlType expected,
            const char* operation) {

            if (!model.isValid()) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": invalid model");
            }

            if (model.type() != expected) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": unexpected model type");
            }
        }

        static std::filesystem::path normalizeOutputBase(
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
                    ": empty output path");
            }

            if (value.find(L'\0') != std::wstring::npos) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": output path contains an embedded null character");
            }

            std::filesystem::path base = outputFile;
            const std::wstring extension =
                base.extension().wstring();

            if (!extension.empty()) {
                std::wstring normalized = extension;
                std::transform(
                    normalized.begin(),
                    normalized.end(),
                    normalized.begin(),
                    [](wchar_t character) {
                        if (character >= L'A' &&
                            character <= L'Z') {
                            return static_cast<wchar_t>(
                                character - L'A' + L'a');
                        }

                        return character;
                    });

                if (normalized == L".stp" ||
                    normalized == L".step") {
                    base.replace_extension();
                } else {
                    throw std::invalid_argument(
                        std::string(operation) +
                        ": output path must be a STEP filename "
                        "(.stp/.step) or a filename without extension");
                }
            }

            const std::wstring baseValue =
                base.wstring();

            if (baseValue.empty()) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": STEP output base name is empty");
            }

            if (baseValue.size() >=
                static_cast<std::size_t>(
                    PRO_PATH_SIZE)) {
                throw std::length_error(
                    std::string(operation) +
                    ": output path exceeds PRO_PATH_SIZE");
            }

            return base;
        }

        static void copyPath(
            const std::filesystem::path& path,
            detail::RawPath output,
            const char* operation) {

            const std::wstring value =
                path.wstring();

            if (value.empty()) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": empty output path");
            }

            if (value.find(L'\0') != std::wstring::npos) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": output path contains an embedded null character");
            }

            if (value.size() >=
                static_cast<std::size_t>(
                    PRO_PATH_SIZE)) {
                throw std::length_error(
                    std::string(operation) +
                    ": output path exceeds PRO_PATH_SIZE");
            }

            std::copy(
                value.begin(),
                value.end(),
                output);

            output[value.size()] = L'\0';
        }
    };

}
