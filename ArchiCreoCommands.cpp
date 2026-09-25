#include "ArchiCreoCommands.hpp"

#include "ArchiJavaFxService.hpp"

#include <unordered_map>
#include <string>
#include <exception>
#include <utility>

namespace {

enum class JavaFxAction {
    CreatePart,
    CreateAssembly
};

std::unordered_map<
    ArchiJavaFxService::RequestId,
    JavaFxAction> pendingActions;

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
                switch (action) {
                case JavaFxAction::CreatePart:
                    createPartFromJavaFx(result);
                    success = true;
                    break;
                case JavaFxAction::CreateAssembly:
                    createAssemblyFromJavaFx(result);
                    success = true;
                    break;
                }
            } catch (const std::exception& exception) {
                error = exception.what();
            } catch (...) {
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
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result)
{
    // TODO: remplacer par la logique ProToolkit de création de pièce.
    // result.values contient les valeurs retournées par JavaFX.
    (void)result;
}

void createAssemblyFromJavaFx(
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result)
{
    // TODO: remplacer par la logique ProToolkit de création d'assemblage.
    // result.values contient les valeurs retournées par JavaFX.
    (void)result;
}
