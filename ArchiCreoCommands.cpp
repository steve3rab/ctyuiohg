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
    // This function is called by Creo's command/UI callback.
    // Keep all Pro/TOOLKIT work that must happen BEFORE the JavaFX dialog here.
    // Example:
    //   ProMdlCurrentGet(...);
    //   ProParameterValueGet(...);
    //   ...
    //
    // Do not move those calls into the JVM thread or JavaFX Application Thread.
    // The JavaFX window is opened only after this Creo-side preparation returns.
    //
    // TODO: add the actual Create Part preparation once its TOOLKIT contract
    // (model, template, parameters, etc.) is defined.

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
    // IMPORTANT: this callback is dispatched back to the Creo thread by
    // ArchiCreoUiDispatcher. Pro/TOOLKIT calls belong here, not on the JVM/FX
    // thread.
    //
    // This is the AFTER-JavaFX phase:
    //   1. JavaFX returns the user's values.
    //   2. This function performs the actual Pro/TOOLKIT operation.
    //   3. The caller completes the request only after this function returns.
    //
    // TODO: replace with the real Pro/TOOLKIT creation workflow.
    (void)result;
}

void createAssemblyFromJavaFx(
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result)
{
    // This callback is also executed on the Creo thread. Keep all
    // Pro/TOOLKIT work in this function.
    //
    // This is the AFTER-JavaFX phase; the JavaFX/JNI threads must never call
    // Pro/TOOLKIT directly.
    // TODO: replace with the real Pro/TOOLKIT assembly workflow.
    (void)result;
}
