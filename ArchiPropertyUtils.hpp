#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>

class ArchiPropertyUtils final {
  public:
    ArchiPropertyUtils() = delete;

    static std::wstring charToWideString(
        const char* text);

    static std::wstring stringToWideString(
        const std::string& text);

    static std::string wideStringToString(
        const std::wstring& text);

    static std::string environmentStr(
        const char* name);

    static std::filesystem::path environmentPath(
        const wchar_t* name);

  private:
    static std::wstring utf8ToWide(
        const char* text);

    static std::string wideToUtf8(
        const std::wstring& text);
};

namespace property_utils_detail {

inline std::wstring readEnvironment(
    const wchar_t* name) {
    if (name == nullptr || *name == L'\0') {
        throw std::invalid_argument(
            "Environment variable name is empty");
    }

    constexpr DWORD kMaxEnvironmentChars = 32767;

    DWORD required =
        GetEnvironmentVariableW(
            name, nullptr, 0);

    if (required == 0) {
        if (GetLastError() ==
            ERROR_ENVVAR_NOT_FOUND) {
            throw std::runtime_error(
                "Required environment variable is missing");
        }

        throw std::runtime_error(
            "Environment variable is empty or cannot be queried");
    }

    for (;;) {
        if (required > kMaxEnvironmentChars) {
            throw std::length_error(
                "Environment variable is too long");
        }

        std::wstring value(
            static_cast<std::size_t>(required),
            L'\0');

        const DWORD written =
            GetEnvironmentVariableW(
                name,
                value.data(),
                required);

        if (written == 0) {
            if (GetLastError() ==
                ERROR_ENVVAR_NOT_FOUND) {
                throw std::runtime_error(
                    "Required environment variable disappeared");
            }

            throw std::runtime_error(
                "Cannot read environment variable");
        }

        if (written >= required) {
            required = written + 1;
            continue;
        }

        value.resize(
            static_cast<std::size_t>(written));

        if (value.empty()) {
            throw std::runtime_error(
                "Environment variable is empty");
        }

        return value;
    }
}

} // namespace property_utils_detail

inline std::wstring ArchiPropertyUtils::utf8ToWide(
    const char* text) {
    if (text == nullptr || *text == '\0') {
        return {};
    }

    if (std::char_traits<char>::length(text) >
        static_cast<std::size_t>(
            (std::numeric_limits<int>::max)())) {
        throw std::length_error(
            "UTF-8 string is too long");
    }

    const int required =
        MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text,
            -1,
            nullptr,
            0);

    if (required <= 0) {
        throw std::invalid_argument(
            "Text UTF-8 is not valid");
    }

    std::wstring result(
        static_cast<std::size_t>(required),
        L'\0');

    if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text,
            -1,
            result.data(),
            required) != required) {
        throw std::runtime_error(
            "Failed to complete UTF-8 conversion");
    }

    if (!result.empty() &&
        result.back() == L'\0') {
        result.pop_back();
    }

    return result;
}

inline std::string ArchiPropertyUtils::wideToUtf8(
    const std::wstring& text) {
    if (text.empty()) {
        return {};
    }

    if (text.size() >
        static_cast<std::size_t>(
            (std::numeric_limits<int>::max)())) {
        throw std::length_error(
            "UTF-16 string is too long");
    }

    const int length =
        static_cast<int>(text.size());

    const int required =
        WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            text.data(),
            length,
            nullptr,
            0,
            nullptr,
            nullptr);

    if (required <= 0) {
        throw std::runtime_error(
            "Failed to convert UTF-16 to UTF-8");
    }

    std::string result(
        static_cast<std::size_t>(required),
        '\0');

    if (WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            text.data(),
            length,
            result.data(),
            required,
            nullptr,
            nullptr) != required) {
        throw std::runtime_error(
            "Failed to complete UTF-16 to UTF-8 conversion");
    }

    return result;
}

inline std::wstring ArchiPropertyUtils::charToWideString(
    const char* text) {
    return utf8ToWide(text);
}

inline std::wstring ArchiPropertyUtils::stringToWideString(
    const std::string& text) {
    return utf8ToWide(text.c_str());
}

inline std::string ArchiPropertyUtils::wideStringToString(
    const std::wstring& text) {
    return wideToUtf8(text);
}

inline std::string ArchiPropertyUtils::environmentStr(
    const char* name) {
    const std::wstring wideName =
        charToWideString(name);

    try {
        return wideStringToString(
            property_utils_detail::readEnvironment(
                wideName.c_str()));
    } catch (const std::exception& error) {
        throw std::runtime_error(
            std::string(error.what()) + " : " +
            (name == nullptr ? "" : name));
    }
}

inline std::filesystem::path
ArchiPropertyUtils::environmentPath(
    const wchar_t* name) {
    try {
        return std::filesystem::path(
            property_utils_detail::readEnvironment(
                name));
    } catch (const std::exception& error) {
        std::string label;

        try {
            label = wideStringToString(
                name == nullptr
                    ? L""
                    : std::wstring(name));
        } catch (...) {
            label = "<unrepresentable>";
        }

        throw std::runtime_error(
            std::string(error.what()) +
            " : " + label);
    }
}
