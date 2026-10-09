#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "ArchiCreoModelHandler.hpp"
#include "ArchiCreoTableHandler.hpp"

namespace creo {

    class ArchiCreoDrawingHandler final {
      public:
        ArchiCreoDrawingHandler() noexcept :
            model_() {
        }

        explicit ArchiCreoDrawingHandler(
            detail::RawMdl handle) noexcept :
            model_(handle) {
        }

        explicit ArchiCreoDrawingHandler(
            const ArchiCreoModelHandler& model) :
            model_(model.raw()) {
            requireDrawing();
        }

        static ArchiCreoDrawingHandler fromCurrentWindow() {
            ArchiCreoModelHandler model =
                ArchiCreoModelHandler::fromCurrentWindow();

            return ArchiCreoDrawingHandler(model);
        }

        static std::optional<ArchiCreoDrawingHandler> findInSession(
            const std::wstring& drawing_name) {
            const auto drawing =
                ArchiCreoModelHandler::findInSession(
                    drawing_name,
                    PRO_MDL_DRAWING);

            if (!drawing.has_value()) {
                return std::nullopt;
            }

            return ArchiCreoDrawingHandler(*drawing);
        }

        [[nodiscard]] detail::RawMdl raw() const noexcept {
            return model_.raw();
        }

        [[nodiscard]] bool isValid() const noexcept {
            return model_.isValid();
        }

        [[nodiscard]] bool isDrawing() const {
            return isValid() &&
                   model_.type() == PRO_MDL_DRAWING;
        }

        std::wstring name() const {
            requireDrawing();
            return model_.name();
        }

        std::wstring directoryPath() const {
            requireDrawing();
            return model_.directoryPath();
        }

        std::wstring extension() const {
            requireDrawing();
            return model_.extension();
        }

        ArchiCreoWindowHandler window() const {
            requireDrawing();
            return model_.window();
        }

        void displayAndActivate() const {
            requireDrawing();
            model_.displayAndActivate();
        }

        void display() const {
            requireDrawing();
            model_.display();
        }

        ArchiCreoWindowHandler displayInNewWindow() const {
            requireDrawing();
            return model_.displayInNewWindow();
        }

        void save() const {
            requireDrawing();
            model_.save();
        }

        [[nodiscard]] ModelInfo info() const {
            requireDrawing();
            return model_.info();
        }

        ArchiCreoTableHandler createTable(
            int rows,
            int columns,
            const detail::RawPoint3d& origin,
            double rowHeight = 1.0,
            const std::vector<double>& columnWidths = {}) const {

            requireDrawing();

            return ArchiCreoTableHandler::create(
                model_,
                rows,
                columns,
                origin,
                rowHeight,
                columnWidths);
        }

        [[nodiscard]] ArchiCreoModelHandler model() const {
            requireDrawing();
            return model_;
        }

      private:
        void requireDrawing() const {
            if (!model_.isValid()) {
                throw std::invalid_argument(
                    "ArchiCreoDrawingHandler: invalid (null) handle");
            }

            if (model_.type() != PRO_MDL_DRAWING) {
                throw std::invalid_argument(
                    "ArchiCreoDrawingHandler: model is not a drawing");
            }
        }

        ArchiCreoModelHandler model_;
    };

}
