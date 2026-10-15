#pragma once

#include "ArchiCreoCompat.hpp"
#include "ArchiCreoErrorHandler.hpp"
#include "ArchiScopeGuard.hpp"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace creo {

    enum class ArchiCreoFileListMode {
        All,
        Latest,
        AllSorted,
        LatestSorted
    };

    struct ArchiCreoFileList {
        std::vector<std::wstring> files;
        std::vector<std::wstring> directories;
    };

    struct ArchiCreoFilePathInfo {
        std::wstring directory;
        std::wstring name;
        std::wstring extension;
        int version = PRO_VALUE_UNUSED;
    };

    struct ArchiCreoFileTypeInfo {
        detail::RawMdlFileType fileType = PRO_MDLFILE_UNUSED;
        detail::RawMdlType modelType = PRO_MDL_UNUSED;
        detail::RawMdlSubtype subtype = PROMDLSTYPE_NONE;
    };

    class ArchiCreoWorkingDirectoryScope final {
      public:
        explicit ArchiCreoWorkingDirectoryScope(
            const std::filesystem::path& directory);

        ArchiCreoWorkingDirectoryScope(
            const ArchiCreoWorkingDirectoryScope&) = delete;

        ArchiCreoWorkingDirectoryScope& operator=(
            const ArchiCreoWorkingDirectoryScope&) = delete;

        ArchiCreoWorkingDirectoryScope(
            ArchiCreoWorkingDirectoryScope&&) = delete;

        ArchiCreoWorkingDirectoryScope& operator=(
            ArchiCreoWorkingDirectoryScope&&) = delete;

        ~ArchiCreoWorkingDirectoryScope() noexcept;

      private:
        std::filesystem::path previous_directory_;
    };

    class ArchiCreoWorkspaceHandler final {
      public:
        ArchiCreoWorkspaceHandler() = delete;

        static std::filesystem::path currentDirectory() {
            detail::RawPath path{};
            CREO_CHECK(
                detail::directoryCurrentGet(path));
            return std::filesystem::path(path);
        }

        static void changeDirectory(
            const std::filesystem::path& directory) {
            const std::wstring value = directory.wstring();

            validatePath(
                value,
                "ArchiCreoWorkspaceHandler::changeDirectory");

            detail::RawPath path{};
            copyWideString(
                value,
                path,
                "ArchiCreoWorkspaceHandler::changeDirectory");

            CREO_CHECK(
                detail::directoryChange(path));
        }

        static ArchiCreoWorkingDirectoryScope scopedDirectory(
            const std::filesystem::path& directory) {
            return ArchiCreoWorkingDirectoryScope(directory);
        }

        static ArchiCreoFileList listFiles(
            const std::filesystem::path& directory = {},
            const std::wstring& filter = {},
            ArchiCreoFileListMode mode =
                ArchiCreoFileListMode::LatestSorted) {

            detail::RawPath directory_buffer{};
            detail::RawPath* file_names = nullptr;
            detail::RawPath* directory_names = nullptr;
            detail::RawLine filter_buffer{};

            const bool has_directory = !directory.empty();
            const bool has_filter = !filter.empty();

            if (has_directory) {
                copyWideString(
                    directory.wstring(),
                    directory_buffer,
                    "ArchiCreoWorkspaceHandler::listFiles");
            }

            if (has_filter) {
                copyWideString(
                    filter,
                    filter_buffer,
                    "ArchiCreoWorkspaceHandler::listFiles");
            }

            auto freeFileNames = Defer([&] {
                (void)detail::arrayFree(
                    reinterpret_cast<detail::RawArray*>(
                        &file_names));
            });

            auto freeDirectoryNames = Defer([&] {
                (void)detail::arrayFree(
                    reinterpret_cast<detail::RawArray*>(
                        &directory_names));
            });

            CREO_CHECK(
                detail::arrayAlloc(
                    0,
                    sizeof(detail::RawPath),
                    1,
                    reinterpret_cast<detail::RawArray*>(
                        &file_names)));

            CREO_CHECK(
                detail::arrayAlloc(
                    0,
                    sizeof(detail::RawPath),
                    1,
                    reinterpret_cast<detail::RawArray*>(
                        &directory_names)));

            CREO_CHECK(
                detail::filesList(
                    has_directory
                        ? directory_buffer
                        : nullptr,
                    has_filter
                        ? filter_buffer
                        : nullptr,
                    toRawListMode(mode),
                    &file_names,
                    &directory_names));

            ArchiCreoFileList result;

            appendArray(
                file_names,
                result.files);

            appendArray(
                directory_names,
                result.directories);

            return result;
        }

        static std::optional<std::filesystem::path> chooseDirectory(
            const std::wstring& dialogLabel = {},
            const std::filesystem::path& defaultDirectory = {}) {

            detail::RawName dialog_label{};
            detail::RawPath default_path{};
            detail::RawPath selected_path{};

            const bool has_label = !dialogLabel.empty();
            const bool has_default = !defaultDirectory.empty();

            if (has_label) {
                copyWideString(
                    dialogLabel,
                    dialog_label,
                    "ArchiCreoWorkspaceHandler::chooseDirectory");
            }

            if (has_default) {
                copyWideString(
                    defaultDirectory.wstring(),
                    default_path,
                    "ArchiCreoWorkspaceHandler::chooseDirectory");
            }

            const detail::ProErrorCode status =
                detail::directoryChoose(
                    has_label ? dialog_label : nullptr,
                    nullptr,
                    nullptr,
                    has_default ? default_path : nullptr,
                    selected_path);

            if (status == PRO_TK_USER_ABORT) {
                return std::nullopt;
            }

            CREO_CHECK(status);

            if (selected_path[0] == L'\0') {
                throw std::runtime_error(
                    "ArchiCreoWorkspaceHandler::chooseDirectory: "
                    "Creo returned an empty path");
            }

            return std::filesystem::path(selected_path);
        }

        static std::optional<std::filesystem::path> chooseFile(
            const std::wstring& filter = {},
            const std::wstring& dialogLabel = {},
            const std::filesystem::path& defaultDirectory = {},
            const std::wstring& preselectedFile = {}) {

            return chooseFileImpl(
                filter,
                dialogLabel,
                defaultDirectory,
                preselectedFile,
                false);
        }

        static std::optional<std::filesystem::path> chooseFileForSave(
            const std::wstring& filter = {},
            const std::wstring& dialogLabel = {},
            const std::filesystem::path& defaultDirectory = {},
            const std::wstring& preselectedFile = {}) {

            return chooseFileImpl(
                filter,
                dialogLabel,
                defaultDirectory,
                preselectedFile,
                true);
        }

        static ArchiCreoFilePathInfo parseFileName(
            const std::filesystem::path& file) {

            const std::wstring value = file.wstring();

            validatePath(
                value,
                "ArchiCreoWorkspaceHandler::parseFileName");

            detail::RawPath file_name_w_path{};
            detail::RawPath directory{};
            detail::RawMdlName name{};
            detail::RawMdlExtension extension{};
            int version = PRO_VALUE_UNUSED;

            copyWideString(
                value,
                file_name_w_path,
                "ArchiCreoWorkspaceHandler::parseFileName");

            CREO_CHECK(
                detail::fileMdlnameParse(
                    file_name_w_path,
                    directory,
                    name,
                    extension,
                    &version));

            ArchiCreoFilePathInfo result;
            result.directory = directory;
            result.name = name;
            result.extension = extension;
            result.version = version;
            return result;
        }

        static std::filesystem::path buildFileName(
            const std::filesystem::path& directory,
            const std::wstring& name,
            const std::wstring& extension,
            int version = PRO_VALUE_UNUSED) {

            if (directory.empty() &&
                name.empty() &&
                extension.empty()) {
                throw std::invalid_argument(
                    "ArchiCreoWorkspaceHandler::buildFileName: "
                    "no file path components");
            }

            detail::RawPath directory_buffer{};
            detail::RawMdlName name_buffer{};
            detail::RawMdlExtension extension_buffer{};
            detail::RawPath result{};

            const bool has_directory = !directory.empty();
            const bool has_name = !name.empty();
            const bool has_extension = !extension.empty();

            if (has_directory) {
                copyWideString(
                    directory.wstring(),
                    directory_buffer,
                    "ArchiCreoWorkspaceHandler::buildFileName");
            }

            if (has_name) {
                copyWideString(
                    name,
                    name_buffer,
                    "ArchiCreoWorkspaceHandler::buildFileName");
            }

            if (has_extension) {
                copyWideString(
                    extension,
                    extension_buffer,
                    "ArchiCreoWorkspaceHandler::buildFileName");
            }

            CREO_CHECK(
                detail::pathMdlnameCreate(
                    has_directory
                        ? directory_buffer
                        : nullptr,
                    has_name
                        ? name_buffer
                        : nullptr,
                    has_extension
                        ? extension_buffer
                        : nullptr,
                    version,
                    result));

            if (result[0] == L'\0') {
                throw std::runtime_error(
                    "ArchiCreoWorkspaceHandler::buildFileName: "
                    "Creo returned an empty path");
            }

            return std::filesystem::path(result);
        }

        static std::optional<ArchiCreoFileTypeInfo> identifyFile(
            const std::filesystem::path& file) {

            const std::wstring value = file.wstring();

            validatePath(
                value,
                "ArchiCreoWorkspaceHandler::identifyFile");

            detail::RawPath file_buffer{};
            copyWideString(
                value,
                file_buffer,
                "ArchiCreoWorkspaceHandler::identifyFile");

            ArchiCreoFileTypeInfo result;

            const detail::ProErrorCode status =
                detail::fileSubtypeGet(
                    file_buffer,
                    &result.fileType,
                    &result.modelType,
                    &result.subtype);

            if (status == PRO_TK_NOT_EXIST) {
                return std::nullopt;
            }

            CREO_CHECK(status);
            return result;
        }

      private:
        static void validatePath(
            const std::wstring& value,
            const char* operation) {

            if (value.empty()) {
                throw std::invalid_argument(
                    std::string(operation) +
                    ": empty path");
            }

            if (value.size() >=
                static_cast<std::size_t>(PRO_PATH_SIZE)) {
                throw std::length_error(
                    std::string(operation) +
                    ": path exceeds PRO_PATH_SIZE");
            }
        }

        template<std::size_t N>
        static void copyWideString(
            const std::wstring& value,
            wchar_t (&destination)[N],
            const char* operation) {

            if (value.size() >= N) {
                throw std::length_error(
                    std::string(operation) +
                    ": string is too long");
            }

            std::copy(
                value.begin(),
                value.end(),
                destination);

            destination[value.size()] = L'\0';
        }

        template<typename T>
        static void appendArray(
            T* values,
            std::vector<std::wstring>& output) {

            if (values == nullptr) {
                return;
            }

            int count = 0;

            CREO_CHECK(
                detail::arraySizeGet(
                    reinterpret_cast<detail::RawArray>(
                        values),
                    &count));

            if (count < 0) {
                throw std::runtime_error(
                    "ArchiCreoWorkspaceHandler: "
                    "Creo returned a negative array size");
            }

            output.reserve(
                static_cast<std::size_t>(count));

            for (int index = 0; index < count; ++index) {
                if (values[index] == nullptr) {
                    continue;
                }

                output.emplace_back(values[index]);
            }
        }

        static detail::RawFileListOpt toRawListMode(
            ArchiCreoFileListMode mode) {

            switch (mode) {
                case ArchiCreoFileListMode::All:
                    return PRO_FILE_LIST_ALL;

                case ArchiCreoFileListMode::Latest:
                    return PRO_FILE_LIST_LATEST;

                case ArchiCreoFileListMode::AllSorted:
                    return PRO_FILE_LIST_ALL_SORTED;

                case ArchiCreoFileListMode::LatestSorted:
                    return PRO_FILE_LIST_LATEST_SORTED;
            }

            throw std::invalid_argument(
                "ArchiCreoWorkspaceHandler: invalid file list mode");
        }

        static std::optional<std::filesystem::path> chooseFileImpl(
            const std::wstring& filter,
            const std::wstring& dialogLabel,
            const std::filesystem::path& defaultDirectory,
            const std::wstring& preselectedFile,
            bool save) {

            detail::RawLine filter_buffer{};
            detail::RawName dialog_label{};
            detail::RawPath default_path{};
            detail::RawMdlFileName preselected{};
            detail::RawPath selected_path{};

            const bool has_filter = !filter.empty();
            const bool has_label = !dialogLabel.empty();
            const bool has_default = !defaultDirectory.empty();
            const bool has_preselected = !preselectedFile.empty();

            if (has_filter) {
                copyWideString(
                    filter,
                    filter_buffer,
                    "ArchiCreoWorkspaceHandler::chooseFile");
            }

            if (has_label) {
                copyWideString(
                    dialogLabel,
                    dialog_label,
                    "ArchiCreoWorkspaceHandler::chooseFile");
            }

            if (has_default) {
                copyWideString(
                    defaultDirectory.wstring(),
                    default_path,
                    "ArchiCreoWorkspaceHandler::chooseFile");
            }

            if (has_preselected) {
                copyWideString(
                    preselectedFile,
                    preselected,
                    "ArchiCreoWorkspaceHandler::chooseFile");
            }

            const detail::ProErrorCode status =
                save
                    ? detail::fileMdlnameSave(
                          has_label ? dialog_label : nullptr,
                          has_filter ? filter_buffer : nullptr,
                          nullptr,
                          nullptr,
                          has_default ? default_path : nullptr,
                          has_preselected ? preselected : nullptr,
                          selected_path)
                    : detail::fileMdlnameOpen(
                          has_label ? dialog_label : nullptr,
                          has_filter ? filter_buffer : nullptr,
                          nullptr,
                          nullptr,
                          has_default ? default_path : nullptr,
                          has_preselected ? preselected : nullptr,
                          selected_path);

            if (status == PRO_TK_USER_ABORT) {
                return std::nullopt;
            }

            CREO_CHECK(status);

            if (selected_path[0] == L'\0') {
                throw std::runtime_error(
                    std::string(
                        "ArchiCreoWorkspaceHandler::") +
                    (save
                        ? "chooseFileForSave"
                        : "chooseFile") +
                    ": Creo returned an empty path");
            }

            return std::filesystem::path(selected_path);
        }
    };

    inline ArchiCreoWorkingDirectoryScope::ArchiCreoWorkingDirectoryScope(
        const std::filesystem::path& directory) :
        previous_directory_(
            ArchiCreoWorkspaceHandler::currentDirectory()) {
        ArchiCreoWorkspaceHandler::changeDirectory(directory);
    }

    inline ArchiCreoWorkingDirectoryScope::~ArchiCreoWorkingDirectoryScope()
        noexcept {
        try {
            ArchiCreoWorkspaceHandler::changeDirectory(
                previous_directory_);
        } catch (...) {
            // Directory restoration is best effort during cleanup.
        }
    }

}
