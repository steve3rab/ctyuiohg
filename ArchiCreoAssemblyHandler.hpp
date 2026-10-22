#pragma once

#include "ArchiCreoCompat.hpp"
#include "ArchiCreoErrorHandler.hpp"
#include "ArchiCreoModelHandler.hpp"

#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace creo {

    struct ArchiCreoAssemblyComponent {
        int featureId = 0;
        detail::RawAsmcompPath componentPath{};
        detail::RawMdl model = nullptr;
        detail::RawMdlType modelType = PRO_MDL_UNUSED;
        bool modelAvailable = false;
        bool isAssembly = false;
    };

    class ArchiCreoAssemblyHandler final {
      public:
        ArchiCreoAssemblyHandler() = delete;

        static std::vector<ArchiCreoAssemblyComponent>
        collectComponents(
            const ArchiCreoModelHandler& assembly,
            bool recursive = true) {

            if (!assembly.isValid()) {
                throw std::invalid_argument(
                    "ArchiCreoAssemblyHandler::collectComponents: "
                    "invalid model");
            }

            if (!assembly.isAssembly()) {
                throw std::invalid_argument(
                    "ArchiCreoAssemblyHandler::collectComponents: "
                    "model is not an assembly");
            }

            TraversalContext context{
                static_cast<detail::RawSolid>(assembly.raw()),
                recursive,
                {},
                {},
                nullptr};

            const detail::ProErrorCode status =
                detail::solidFeatVisit(
                    context.rootAssembly,
                    &visitAction,
                    &filterAction,
                    &context);

            if (context.exception) {
                std::rethrow_exception(
                    context.exception);
            }

            if (status == PRO_TK_E_NOT_FOUND) {
                return {};
            }

            CREO_CHECK(status);
            return std::move(context.components);
        }

      private:
        struct TraversalContext {
            detail::RawSolid rootAssembly = nullptr;
            bool recursive = true;
            std::vector<int> componentIds;
            std::vector<ArchiCreoAssemblyComponent> components;
            std::exception_ptr exception;
        };

        static detail::ProErrorCode filterAction(
            detail::RawFeature* feature,
            detail::RawAppData appData) noexcept {

            TraversalContext* context =
                static_cast<TraversalContext*>(appData);

            if (context == nullptr || feature == nullptr) {
                return PRO_TK_BAD_INPUTS;
            }

            try {
                detail::RawFeatureType featureType =
                    0;

                CREO_CHECK(
                    detail::featureTypeGet(
                        feature,
                        &featureType));

                return featureType == PRO_FEAT_COMPONENT
                    ? PRO_TK_NO_ERROR
                    : PRO_TK_CONTINUE;
            }
            catch (...) {
                context->exception =
                    std::current_exception();

                return PRO_TK_GENERAL_ERROR;
            }
        }

        static detail::ProErrorCode visitAction(
            detail::RawFeature* feature,
            detail::ProErrorCode status,
            detail::RawAppData appData) noexcept {

            TraversalContext* context =
                static_cast<TraversalContext*>(appData);

            if (context == nullptr || feature == nullptr) {
                return PRO_TK_BAD_INPUTS;
            }

            if (status != PRO_TK_NO_ERROR) {
                return status;
            }

            try {
                auto* component =
                    reinterpret_cast<detail::RawAsmcomp*>(
                        feature);

                const int featureId =
                    feature->id;

                if (featureId <= 0) {
                    throw std::runtime_error(
                        "ArchiCreoAssemblyHandler: "
                        "Creo returned an invalid component feature id");
                }

                if (context->componentIds.size() >=
                    static_cast<std::size_t>(
                        PRO_MAX_ASSEM_LEVEL)) {
                    throw std::runtime_error(
                        "ArchiCreoAssemblyHandler: "
                        "assembly nesting exceeds PRO_MAX_ASSEM_LEVEL");
                }

                context->componentIds.push_back(
                    featureId);

                auto popId = makePopGuard(
                    context->componentIds);

                detail::RawAsmcompPath path{};
                CREO_CHECK(
                    detail::asmcomppathInit(
                        context->rootAssembly,
                        context->componentIds.data(),
                        static_cast<int>(
                            context->componentIds.size()),
                        &path));

                detail::RawMdl componentModel = nullptr;

                const detail::ProErrorCode modelStatus =
                    detail::asmcompMdlGet(
                        component,
                        &componentModel);

                ArchiCreoAssemblyComponent result;
                result.featureId = featureId;
                result.componentPath = path;

                if (modelStatus == PRO_TK_E_NOT_FOUND) {
                    result.modelAvailable = false;
                    result.model = nullptr;
                    result.modelType = PRO_MDL_UNUSED;
                    result.isAssembly = false;

                    context->components.push_back(
                        result);
                    return PRO_TK_NO_ERROR;
                }

                CREO_CHECK(modelStatus);

                if (componentModel == nullptr) {
                    throw std::runtime_error(
                        "ArchiCreoAssemblyHandler: "
                        "Creo returned a null component model");
                }

                result.model = componentModel;
                result.modelAvailable = true;

                CREO_CHECK(
                    detail::mdlTypeGet(
                        componentModel,
                        &result.modelType));

                result.isAssembly =
                    result.modelType == PRO_MDL_ASSEMBLY;

                context->components.push_back(
                    result);

                if (context->recursive &&
                    result.isAssembly) {
                    const auto nestedStatus =
                        detail::solidFeatVisit(
                            static_cast<detail::RawSolid>(
                                componentModel),
                            &visitAction,
                            &filterAction,
                            context);

                    if (nestedStatus != PRO_TK_NO_ERROR &&
                        nestedStatus != PRO_TK_E_NOT_FOUND) {
                        return nestedStatus;
                    }
                }

                return PRO_TK_NO_ERROR;
            }
            catch (...) {
                context->exception =
                    std::current_exception();

                return PRO_TK_GENERAL_ERROR;
            }
        }

        class PopIdGuard final {
          public:
            explicit PopIdGuard(
                std::vector<int>& ids) noexcept :
                ids_(ids) {
            }

            PopIdGuard(
                const PopIdGuard&) = delete;

            PopIdGuard& operator=(
                const PopIdGuard&) = delete;

            ~PopIdGuard() noexcept {
                if (!ids_.empty()) {
                    ids_.pop_back();
                }
            }

          private:
            std::vector<int>& ids_;
        };

        static PopIdGuard makePopGuard(
            std::vector<int>& ids) noexcept {
            return PopIdGuard(ids);
        }
    };

}
