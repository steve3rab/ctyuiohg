#include "ArchiCreoCommands.hpp"

#include "ArchiCreoModelHandler.hpp"
#include "ArchiJavaFxService.hpp"
#include "ArchiPropertyUtils.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
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
                return !isSpace(
                    static_cast<unsigned char>(
                        character));
            }));

    value.erase(
        std::find_if(
            value.rbegin(),
            value.rend(),
            [&](char character) {
                return !isSpace(
                    static_cast<unsigned char>(
                        character));
            }).base(),
        value.end());

    return value;
}

std::string getModelName(
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result)
{
    if (result.values.empty()) {
        throw std::runtime_error(
            "JavaFX returned no model name.");
    }

    std::string modelName =
        trim(result.values.front());

    if (modelName.empty()) {
        throw std::runtime_error(
            "JavaFX returned an empty model name.");
    }

    return modelName;
}

} // namespace

void configureJavaFxResultCallback()
{
    ArchiJavaFxService::instance().setResultCallback(
        [](jnifx::ArchiJavaFxRuntime::JavaFxResult result)
        {
            const auto it =
                pendingActions.find(result.requestId);

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
                const std::string modelName =
                    getModelName(result);

                switch (action) {
                case JavaFxAction::CreatePart:
                    createPartFromJavaFx(
                        modelName, result);
                    success = true;
                    break;

                case JavaFxAction::CreateAssembly:
                    createAssemblyFromJavaFx(
                        modelName, result);
                    success = true;
                    break;
                }
            } catch (const std::exception& exception) {
                error = exception.what();
            } catch (...) {
                error = "Unknown Creo processing error";
            }

            ArchiJavaFxService::instance()
                .completeProcessing(
                    result.requestId,
                    success,
                    std::move(error));
        });
}

jnifx::ArchiJavaFxRuntime::RequestId onCreatePart()
{
    const auto requestId =
        ArchiJavaFxService::instance()
            .openModalWindow(
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
        ArchiJavaFxService::instance()
            .openModalWindow(
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

    const std::wstring modelNameWide =
        ArchiPropertyUtils::stringToWideString(
            modelName);

    const std::filesystem::path applicationHome =
        ArchiPropertyUtils::environmentPath(
            L"Archi_TOOLS");

    // The template location is application configuration.
    // The destination remains explicit so the caller controls where
    // the copied Creo model is created.
    const std::filesystem::path templatePath =
        applicationHome / "templates" / "part.prt";

    const std::filesystem::path destinationDirectory =
        std::filesystem::current_path();

    const creo::ArchiCreoModelHandler part =
        creo::ArchiCreoModelHandler::createPartFromTemplate(
            templatePath,
            destinationDirectory,
            modelNameWide);

    // The template is loaded by Creo, copied under modelName,
    // and the resulting model handle is returned.
    if (!part.isValid()) {
        throw std::runtime_error(
            "Creo created an invalid part handle.");
    }
}

void createAssemblyFromJavaFx(
    const std::string& modelName,
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result)
{
    (void)result;

    const std::wstring modelNameWide =
        ArchiPropertyUtils::stringToWideString(
            modelName);

    const creo::ArchiCreoModelHandler assembly =
        creo::ArchiCreoModelHandler::createAssembly(
            modelNameWide);

    if (!assembly.isValid()) {
        throw std::runtime_error(
            "Creo created an invalid assembly handle.");
    }
}
