/*
 * Example: enumerate the components of the current Creo assembly.
 *
 * With recursive == true, components are returned in traversal order,
 * including components inside subassemblies.
 *
 * This example intentionally performs no modification of the assembly.
 */

#include "ArchiCreoAssemblyHandler.hpp"
#include "ArchiCreoModelHandler.hpp"
#include "ArchiCreoCompat.hpp"

#include <exception>
#include <stdexcept>
#include <vector>

class ArchiCreoAssemblyExample final {
  public:
    ArchiCreoAssemblyExample() = delete;

    static std::vector<creo::ArchiCreoAssemblyComponent>
    collectCurrentAssemblyComponents(
        bool recursive = true) {

        const auto assembly =
            creo::ArchiCreoModelHandler::fromCurrentWindow();

        return creo::ArchiCreoAssemblyHandler::collectComponents(
            assembly,
            recursive);
    }

    static std::size_t
    countCurrentAssemblyComponents() {

        return collectCurrentAssemblyComponents(true).size();
    }

    /*
     * Direct ProSolidFeatVisit() example.
     *
     * This intentionally demonstrates the native Toolkit traversal
     * separately from ArchiCreoAssemblyHandler.
     */
    static std::size_t
    countCurrentAssemblyComponentsWithSolidFeatVisit() {

        const auto assembly =
            creo::ArchiCreoModelHandler::fromCurrentWindow();

        if (!assembly.isAssembly()) {
            throw std::invalid_argument(
                "countCurrentAssemblyComponentsWithSolidFeatVisit: "
                "current model is not an assembly");
        }

        VisitContext context{};

        const creo::detail::ProErrorCode status =
            creo::detail::solidFeatVisit(
                static_cast<creo::detail::RawSolid>(
                    assembly.raw()),
                &featureVisitAction,
                &componentFilterAction,
                &context);

        if (context.error) {
            std::rethrow_exception(context.error);
        }

        if (status == PRO_TK_E_NOT_FOUND) {
            return 0;
        }

        CREO_CHECK(status);
        return context.componentCount;
    }

  private:
    struct VisitContext {
        std::size_t componentCount = 0;
        std::exception_ptr error;
    };

    static creo::detail::ProErrorCode
    componentFilterAction(
        creo::detail::RawFeature* feature,
        creo::detail::RawAppData appData) noexcept {

        auto* context =
            static_cast<VisitContext*>(appData);

        if (context == nullptr ||
            feature == nullptr) {
            return PRO_TK_BAD_INPUTS;
        }

        try {
            creo::detail::RawFeatureType featureType =
                0;

            CREO_CHECK(
                creo::detail::featureTypeGet(
                    feature,
                    &featureType));

            return featureType == PRO_FEAT_COMPONENT
                ? PRO_TK_NO_ERROR
                : PRO_TK_CONTINUE;
        }
        catch (...) {
            context->error =
                std::current_exception();

            return PRO_TK_GENERAL_ERROR;
        }
    }

    static creo::detail::ProErrorCode
    featureVisitAction(
        creo::detail::RawFeature* feature,
        creo::detail::ProErrorCode status,
        creo::detail::RawAppData appData) noexcept {

        auto* context =
            static_cast<VisitContext*>(appData);

        if (context == nullptr ||
            feature == nullptr) {
            return PRO_TK_BAD_INPUTS;
        }

        if (status != PRO_TK_NO_ERROR) {
            return status;
        }

        try {
            ++context->componentCount;
            return PRO_TK_NO_ERROR;
        }
        catch (...) {
            context->error =
                std::current_exception();

            return PRO_TK_GENERAL_ERROR;
        }
    }
};
