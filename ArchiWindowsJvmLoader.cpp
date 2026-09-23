#include "ArchiWindowsJvmLoader.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace jnifx::detail {
    namespace {

        constexpr char kClassPathSeparator = ';';

        template<typename Function>
        Function loadProcedure(HMODULE module, const char* name) noexcept {
            static_assert(sizeof(Function) == sizeof(FARPROC), "Windows function pointer size mismatch");
            const FARPROC procedure = GetProcAddress(module, name);
            Function function = nullptr;
            std::memcpy(&function, &procedure, sizeof(function));
            return function;
        }

        std::wstring utf8ToWide(const std::string& text) {
            if (text.empty()) {
                return {};
            }
            if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                throw std::length_error("Path UTF-8 too long");
            }

            const int required =
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
            if (required <= 0) {
                throw std::invalid_argument("Path UTF-8 invalid");
            }

            std::wstring result(static_cast<std::size_t>(required), L'\0');
            if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(),
                    required) != required) {
                throw std::runtime_error("Windows impossible to convert");
            }
            return result;
        }

        std::filesystem::path defaultJvmPath(const std::string& environmentVariable) {
            const std::wstring wideVariable = utf8ToWide(environmentVariable);
            const DWORD required = GetEnvironmentVariableW(wideVariable.c_str(), nullptr, 0);
            if (required == 0) {
                throw std::runtime_error("JRE missing et Config.jvmPath not set");
            }

            std::wstring javaHome(required, L'\0');
            const DWORD copied = GetEnvironmentVariableW(wideVariable.c_str(), javaHome.data(), required);
            if (copied == 0 || copied >= required) {
                throw std::runtime_error("JRE cannot be read");
            }
            javaHome.resize(copied);
            return std::filesystem::path(javaHome) / L"bin" / L"server" / L"jvm.dll";
        }

    }    // namespace

    std::string expandClassPath(const std::string& classPath) {
        std::istringstream input(classPath);
        std::string entry;
        std::vector<std::string> expanded;

        while (std::getline(input, entry, kClassPathSeparator)) {
            if (!entry.empty() && entry.back() == '*') {
                const std::filesystem::path directory = std::filesystem::path(entry.substr(0, entry.size() - 1));
                std::vector<std::filesystem::path> jars;
                for (const auto& item : std::filesystem::directory_iterator(directory)) {
                    std::string extension = item.path().extension().string();
                    std::transform(extension.begin(), extension.end(), extension.begin(),
                        [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
                    if (item.is_regular_file() && extension == ".jar") {
                        jars.push_back(item.path());
                    }
                }
                if (jars.empty()) {
                    throw std::runtime_error("No JAR found in : " + directory.string());
                }
                std::sort(jars.begin(), jars.end());
                for (const auto& jar : jars) {
                    expanded.push_back(jar.string());
                }
            } else if (!entry.empty()) {
                expanded.push_back(entry);
            }
        }

        std::string result;
        for (const auto& item : expanded) {
            if (!result.empty()) {
                result += kClassPathSeparator;
            }
            result += item;
        }
        return result;
    }

    ArchiWindowsJvmLoader::~ArchiWindowsJvmLoader() {
        unload();
    }

    void ArchiWindowsJvmLoader::load(const std::string& configuredJvmPath, const std::string& jreEnvironmentVariable) {
        if (module_ != nullptr) {
            return;
        }

        std::filesystem::path path = configuredJvmPath.empty() ? defaultJvmPath(jreEnvironmentVariable) :
                                                                 std::filesystem::path(utf8ToWide(configuredJvmPath));
        path = std::filesystem::absolute(path).lexically_normal();
        if (!std::filesystem::is_regular_file(path)) {
            throw std::runtime_error("jvm.dll missing : " + path.string());
        }

        const std::filesystem::path serverDirectory = path.parent_path();
        const std::filesystem::path binDirectory = serverDirectory.parent_path();
        serverDirectoryCookie_ = AddDllDirectory(serverDirectory.c_str());
        binDirectoryCookie_ = AddDllDirectory(binDirectory.c_str());
        if (serverDirectoryCookie_ == nullptr || binDirectoryCookie_ == nullptr) {
            const DWORD error = GetLastError();
            unload();
            throw std::runtime_error("AddDllDirectory has failed, code Windows " + std::to_string(error));
        }

        DWORD previousErrorMode = 0;
        SetThreadErrorMode(SEM_FAILCRITICALERRORS, &previousErrorMode);
        module_ = LoadLibraryExW(path.c_str(), nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        const DWORD loadError = GetLastError();
        SetThreadErrorMode(previousErrorMode, nullptr);

        if (module_ == nullptr) {
            unload();
            throw std::runtime_error("Loading jvm.dll is impossible, code Windows " + std::to_string(loadError));
        }

        createJavaVm_ = loadProcedure<CreateJavaVmFunction>(module_, "JNI_CreateJavaVM");
        getCreatedJavaVms_ = loadProcedure<GetCreatedJavaVmsFunction>(module_, "JNI_GetCreatedJavaVMs");
        if (createJavaVm_ == nullptr || getCreatedJavaVms_ == nullptr) {
            unload();
            throw std::runtime_error("Exports JNI not found in jvm.dll");
        }
    }

    void ArchiWindowsJvmLoader::unload() noexcept {
        createJavaVm_ = nullptr;
        getCreatedJavaVms_ = nullptr;
        if (module_ != nullptr) {
            FreeLibrary(module_);
            module_ = nullptr;
        }
        if (serverDirectoryCookie_ != nullptr) {
            RemoveDllDirectory(serverDirectoryCookie_);
            serverDirectoryCookie_ = nullptr;
        }
        if (binDirectoryCookie_ != nullptr) {
            RemoveDllDirectory(binDirectoryCookie_);
            binDirectoryCookie_ = nullptr;
        }
    }

    jint ArchiWindowsJvmLoader::getCreatedJavaVms(JavaVM** buffer, jsize bufferLength, jsize* count) const {
        if (getCreatedJavaVms_ == nullptr) {
            return JNI_ERR;
        }
        return getCreatedJavaVms_(buffer, bufferLength, count);
    }

    jint ArchiWindowsJvmLoader::createJavaVm(JavaVM** vm, JNIEnv** env, JavaVMInitArgs* arguments) const {
        if (createJavaVm_ == nullptr) {
            return JNI_ERR;
        }
        return createJavaVm_(vm, reinterpret_cast<void**>(env), arguments);
    }

}    // namespace jnifx::detail
