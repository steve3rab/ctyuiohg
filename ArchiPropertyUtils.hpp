#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <windows.h>

class ArchiPropertyUtils final {
  public:
    ArchiPropertyUtils() = delete;

    static std::wstring charToWideString(const char* text);
    static std::wstring stringToWideString(const std::string& text);
    static std::string wideStringToString(const std::wstring& text);

    static std::string environmentStr(const char* name);
    static std::filesystem::path environmentPath(const wchar_t* name);

  private:
    static std::wstring utf8ToWide(const char* text);
    static std::string wideToUtf8(const std::wstring& text);
};

namespace property_utils_detail {

inline std::wstring readEnvironment(const wchar_t* name) {
    if (name == nullptr || *name == L'\0') {
        throw std::invalid_argument("Variable name empty");
    }

    const DWORD required = GetEnvironmentVariableW(name, nullptr, 0);
    if (required == 0) {
        if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
            throw std::runtime_error("Required environment variable is missing");
        }
        throw std::runtime_error("Cannot query environment variable");
    }

    std::wstring value(static_cast<std::size_t>(required), L'\0');
    const DWORD written =
        GetEnvironmentVariableW(name, value.data(), required);

    if (written == 0 || written >= required) {
        throw std::runtime_error("Cannot read environment variable");
    }

    value.resize(written);

    if (value.empty()) {
        throw std::runtime_error("Empty environment variable");
    }

    return value;
}

}

inline std::wstring ArchiPropertyUtils::utf8ToWide(const char* text) {
    if (text == nullptr || *text == '\0') {
        return {};
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
        throw std::invalid_argument("Text UTF-8 not valid");
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
        throw std::runtime_error("Impossible to complete conversion");
    }

    if (!result.empty() && result.back() == L'\0') {
        result.pop_back();
    }

    return result;
}

inline std::string ArchiPropertyUtils::wideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }

    if (text.size() >
        static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
        throw std::length_error("Variable too long");
    }

    const int required =
        WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

    if (required <= 0) {
        throw std::runtime_error(
            "Impossible to convert UTF-16 to UTF-8");
    }

    std::string result(
        static_cast<std::size_t>(required),
        '\0');

    if (WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            result.data(),
            required,
            nullptr,
            nullptr) != required) {
        throw std::runtime_error(
            "Impossible to complete conversion UTF-16 to UTF-8");
    }

    return result;
}

inline std::wstring ArchiPropertyUtils::charToWideString(const char* text) {
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

inline std::string ArchiPropertyUtils::environmentStr(const char* name) {
    const std::wstring wideName = charToWideString(name);

    try {
        return wideStringToString(
            property_utils_detail::readEnvironment(wideName.c_str()));
    } catch (const std::exception& error) {
        throw std::runtime_error(
            std::string(error.what()) + " : " +
            (name == nullptr ? "" : name));
    }
}

inline std::filesystem::path ArchiPropertyUtils::environmentPath(
    const wchar_t* name) {
    try {
        return std::filesystem::path(
            property_utils_detail::readEnvironment(name));
    } catch (const std::exception& error) {
        std::string label;

        try {
            label = wideStringToString(
                name == nullptr ? L"" : std::wstring(name));
        } catch (...) {
            label = "<unrepresentable>";
        }

        throw std::runtime_error(
            std::string(error.what()) + " : " + label);
    }
}
