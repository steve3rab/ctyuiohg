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

const std::filesystem::path& templateDirectory()
{
    static const std::filesystem::path directory =
        ArchiPropertyUtils::environmentPath(
            L"Archi_TOOLS") / "templates";
    return directory;
}

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

std::string getTemplatePath(
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result)
{
    if (result.values.size() < 2) {
        throw std::runtime_error(
            "JavaFX returned no template path.");
    }

    const std::string templatePath =
        trim(result.values[1]);

    if (templatePath.empty()) {
        throw std::runtime_error(
            "JavaFX returned an empty template path.");
    }

    return templatePath;
}

void refreshCreoUiBestEffort()
{
    try {
        const auto model =
            creo::ArchiCreoModelHandler::fromCurrentWindow();

        if (model.isValid()) {
            model.refreshAfterCreation();
        }
    } catch (...) {
        // Refresh is cleanup only. Never hide the original processing error.
    }
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

            if (result.status !=
                jnifx::ArchiJavaFxRuntime::JavaFxResult::Status::Accepted) {
                pendingActions.erase(it);
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
                        modelName,
                        getTemplatePath(result),
                        result);
                    success = true;
                    break;

                case JavaFxAction::CreateAssembly:
                    createAssemblyFromJavaFx(
                        modelName,
                        getTemplatePath(result),
                        result);
                    success = true;
                    break;
                }
            } catch (const std::exception& exception) {
                error = exception.what();
            } catch (...) {
                error = "Unknown Creo processing error";
            }

            if (!success) {
                refreshCreoUiBestEffort();
            }

            if (success) {
                pendingActions.erase(it);
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
    const auto& templateDirectoryPath =
        templateDirectory();

    const std::string templateDirectoryUtf8 =
        ArchiPropertyUtils::wideStringToString(
            templateDirectoryPath.wstring());

    const auto requestId =
        ArchiJavaFxService::instance()
            .openModalWindow(
                "Create Part",
                {
                    "CREATE_PART",
                    templateDirectoryUtf8
                });

    if (requestId != 0) {
        pendingActions.emplace(
            requestId,
            JavaFxAction::CreatePart);
    }

    return requestId;
}

jnifx::ArchiJavaFxRuntime::RequestId onCreateAssembly()
{
    const auto& templateDirectoryPath =
        templateDirectory();

    const std::string templateDirectoryUtf8 =
        ArchiPropertyUtils::wideStringToString(
            templateDirectoryPath.wstring());

    const auto requestId =
        ArchiJavaFxService::instance()
            .openModalWindow(
                "Create Assembly",
                {
                    "CREATE_ASSEMBLY",
                    templateDirectoryUtf8
                });

    if (requestId != 0) {
        pendingActions.emplace(
            requestId,
            JavaFxAction::CreateAssembly);
    }

    return requestId;
}

void createPartFromJavaFx(
    const std::string& modelName,
    const std::string& templatePath,
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result)
{
    (void)result;

    const std::wstring modelNameWide =
        ArchiPropertyUtils::stringToWideString(
            modelName);

    const std::filesystem::path selectedTemplate =
        std::filesystem::path(
            ArchiPropertyUtils::stringToWideString(
                templatePath));

    const auto& templateDirectoryPath =
        templateDirectory();

    const std::filesystem::path normalizedTemplate =
        std::filesystem::weakly_canonical(
            selectedTemplate);
    const std::filesystem::path normalizedDirectory =
        std::filesystem::weakly_canonical(
            templateDirectoryPath);

    const auto relative =
        std::filesystem::relative(
            normalizedTemplate,
            normalizedDirectory);

    const auto firstComponent =
        relative.empty()
            ? std::filesystem::path{}
            : *relative.begin();

    if (relative.empty() ||
        relative == std::filesystem::path(".") ||
        relative.is_absolute() ||
        firstComponent == std::filesystem::path("..")) {
        throw std::invalid_argument(
            "Selected template is outside the configured template directory.");
    }

    const std::filesystem::path destinationDirectory =
        creo::ArchiCreoModelHandler::creoWorkingDirectory();

    const creo::ArchiCreoModelHandler part =
        creo::ArchiCreoModelHandler::createPartFromTemplate(
            normalizedTemplate,
            destinationDirectory,
            modelNameWide);

    // The template is loaded by Creo, copied under modelName,
    // and the resulting model handle is returned.
    if (!part.isValid()) {
        throw std::runtime_error(
            "Creo created an invalid part handle.");
    }

    part.displayAndActivate();
    part.refreshAfterCreation();
}

void createAssemblyFromJavaFx(
    const std::string& modelName,
    const std::string& templatePath,
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result)
{
    (void)result;

    const std::wstring modelNameWide =
        ArchiPropertyUtils::stringToWideString(
            modelName);

    const std::filesystem::path selectedTemplate =
        std::filesystem::path(
            ArchiPropertyUtils::stringToWideString(
                templatePath));

    const auto& templateDirectoryPath =
        templateDirectory();

    const std::filesystem::path normalizedTemplate =
        std::filesystem::weakly_canonical(
            selectedTemplate);
    const std::filesystem::path normalizedDirectory =
        std::filesystem::weakly_canonical(
            templateDirectoryPath);

    const auto relative =
        std::filesystem::relative(
            normalizedTemplate,
            normalizedDirectory);

    const auto firstComponent =
        relative.empty()
            ? std::filesystem::path{}
            : *relative.begin();

    if (relative.empty() ||
        relative == std::filesystem::path(".") ||
        relative.is_absolute() ||
        firstComponent == std::filesystem::path("..")) {
        throw std::invalid_argument(
            "Selected template is outside the configured template directory.");
    }

    const std::filesystem::path destinationDirectory =
        creo::ArchiCreoModelHandler::creoWorkingDirectory();

    const creo::ArchiCreoModelHandler assembly =
        creo::ArchiCreoModelHandler::createAssemblyFromTemplate(
            normalizedTemplate,
            destinationDirectory,
            modelNameWide);

    if (!assembly.isValid()) {
        throw std::runtime_error(
            "Creo created an invalid assembly handle.");
    }

    assembly.displayAndActivate();
    assembly.refreshAfterCreation();
}
