#include "ArchiCreoCommands.hpp"

#include "ArchiCreoModelHandler.hpp"
#include "ArchiJavaFxService.hpp"

#ifndef _WIN32
#error ArchiCreoCommands requires Windows for UTF-8 to Creo wide-string conversion.
#endif

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace {

enum class JavaFxAction {
    CreatePart,
    CreateAssembly
};

std::unordered_map<
    ArchiJavaFxService::RequestId,
    JavaFxAction> pendingActions;

std::string trim(std::string value)
{
    const auto isSpace = [](unsigned char character) {
        return std::isspace(character) != 0;
    };

    value.erase(
        value.begin(),
        std::find_if(
            value.begin(),
            value.end(),
            [&](char character) {
                return !isSpace(static_cast<unsigned char>(character));
            }));

    value.erase(
        std::find_if(
            value.rbegin(),
            value.rend(),
            [&](char character) {
                return !isSpace(static_cast<unsigned char>(character));
            }).base(),
        value.end());

    return value;
}

std::string getModelName(
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result)
{
    if (result.values.empty()) {
        throw std::runtime_error("JavaFX returned no model name.");
    }

    std::string modelName = trim(result.values.front());

    if (modelName.empty()) {
        throw std::runtime_error("JavaFX returned an empty model name.");
    }

    return modelName;
}

std::wstring utf8ToWide(const std::string& value)
{
    if (value.empty()) {
        return {};
    }

    if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("Model name is too long to convert from UTF-8.");
    }

    const int sourceLength = static_cast<int>(value.size());
    const int required = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        sourceLength,
        nullptr,
        0);

    if (required <= 0) {
        throw std::runtime_error("Model name is not valid UTF-8.");
    }

    std::wstring result(static_cast<std::size_t>(required), L'\0');

    if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            value.data(),
            sourceLength,
            result.data(),
            required) != required) {
        throw std::runtime_error("Failed to convert model name from UTF-8.");
    }

    return result;
}

} // namespace

void configureJavaFxResultCallback()
{
    ArchiJavaFxService::instance().setResultCallback(
        [](jnifx::ArchiJavaFxRuntime::JavaFxResult result)
        {
            const auto it = pendingActions.find(result.requestId);

            if (it == pendingActions.end()) {
                return;
            }

            const JavaFxAction action = it->second;
            pendingActions.erase(it);

            if (result.status !=
                jnifx::ArchiJavaFxRuntime::JavaFxResult::Status::Accepted) {
                return;
            }

            bool success = false;
            std::string error;

            try {
                const std::string modelName = getModelName(result);

                switch (action) {
                case JavaFxAction::CreatePart:
                    createPartFromJavaFx(modelName, result);
                    success = true;
                    break;

                case JavaFxAction::CreateAssembly:
                    createAssemblyFromJavaFx(modelName, result);
                    success = true;
                    break;
                }
            }
            catch (const std::exception& exception) {
                error = exception.what();
            }
            catch (...) {
                error = "Unknown Creo processing error";
            }

            ArchiJavaFxService::instance().completeProcessing(
                result.requestId,
                success,
                std::move(error));
        });
}

jnifx::ArchiJavaFxRuntime::RequestId onCreatePart()
{
    const auto requestId =
        ArchiJavaFxService::instance().openModalWindow(
            "Create Part",
            {"CREATE_PART"});

    if (requestId != 0) {
        pendingActions.emplace(
            requestId,
            JavaFxAction::CreatePart);
    }

    return requestId;
}

jnifx::ArchiJavaFxRuntime::RequestId onCreateAssembly()
{
    const auto requestId =
        ArchiJavaFxService::instance().openModalWindow(
            "Create Assembly",
            {"CREATE_ASSEMBLY"});

    if (requestId != 0) {
        pendingActions.emplace(
            requestId,
            JavaFxAction::CreateAssembly);
    }

    return requestId;
}

void createPartFromJavaFx(
    const std::string& modelName,
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result)
{
    (void)result;

    const std::wstring modelNameWide = utf8ToWide(modelName);
    const creo::ArchiCreoModelHandler part =
        creo::ArchiCreoModelHandler::createPart(modelNameWide);

    // The model is now created in the Creo session. Keep it available for the
    // next phase (parameters/template-specific processing). Do not save here.
    if (!part.isValid()) {
        throw std::runtime_error("Creo created an invalid part handle.");
    }
}

void createAssemblyFromJavaFx(
    const std::string& modelName,
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result)
{
    (void)modelName;
    (void)result;

    // Assembly creation will use the same ModelHandler pattern.
}
