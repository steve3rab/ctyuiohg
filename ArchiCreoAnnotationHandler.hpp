#pragma once

#include <stdexcept>

#include "ArchiCreoCompat.hpp"
#include "ArchiCreoErrorHandler.hpp"

namespace creo {

    struct ArchiCreoAnnotationAssociativity final {
        bool position = false;
        detail::RawAnnotationAttachmentAssociativity attachment =
            PRO_ANNOTATTACH_ASSOCIATIVITY_NONE;
    };

    class ArchiCreoAnnotationHandler final {
      public:
        ArchiCreoAnnotationHandler() noexcept :
            annotation_{},
            valid_(false) {
        }

        explicit ArchiCreoAnnotationHandler(
            detail::RawAnnotation annotation) noexcept :
            annotation_(annotation),
            valid_(true) {
        }

        [[nodiscard]] bool isValid() const noexcept {
            return valid_;
        }

        [[nodiscard]] detail::RawAnnotation raw() const noexcept {
            return annotation_;
        }

        void show(
            detail::RawView view,
            detail::RawAsmcompPath* componentPath = nullptr) const {
            requireAnnotation();

            CREO_CHECK(
                detail::annotationShow(
                    &annotation_,
                    componentPath,
                    view));
        }

        [[nodiscard]] bool isShown(
            detail::RawDrawing drawing) const {
            requireAnnotation();

            detail::RawBoolean shown = PRO_B_FALSE;

            CREO_CHECK(
                detail::annotationIsShown(
                    &annotation_,
                    drawing,
                    &shown));

            return shown == PRO_B_TRUE;
        }

        void display(
            detail::RawDrawing drawing,
            detail::RawView view,
            detail::RawAsmcompPath* componentPath = nullptr) const {
            requireAnnotation();

            CREO_CHECK(
                detail::annotationDisplay(
                    &annotation_,
                    componentPath,
                    drawing,
                    view));
        }

        void undisplay(
            detail::RawDrawing drawing,
            detail::RawAsmcompPath* componentPath = nullptr) const {
            requireAnnotation();

            CREO_CHECK(
                detail::annotationUndisplay(
                    &annotation_,
                    componentPath,
                    drawing));
        }

        void eraseFromDrawing(
            detail::RawDrawing drawing) const {
            requireAnnotation();

            CREO_CHECK(
                detail::drawingAnnotationErase(
                    drawing,
                    &annotation_));
        }

        void update(
            detail::RawDrawing drawing) const {
            requireAnnotation();

            CREO_CHECK(
                detail::annotationUpdate(
                    &annotation_,
                    drawing));
        }

        void updatePosition(
            detail::RawDrawing drawing) const {
            requireAnnotation();

            CREO_CHECK(
                detail::annotationPositionUpdate(
                    &annotation_,
                    drawing));
        }

        void updateAttachment(
            detail::RawDrawing drawing) const {
            requireAnnotation();

            CREO_CHECK(
                detail::annotationAttachmentUpdate(
                    &annotation_,
                    drawing));
        }

        [[nodiscard]] bool isInactive() const {
            requireAnnotation();

            detail::RawBoolean inactive = PRO_B_FALSE;

            CREO_CHECK(
                detail::annotationIsInactive(
                    &annotation_,
                    &inactive));

            return inactive == PRO_B_TRUE;
        }

        [[nodiscard]] ArchiCreoAnnotationAssociativity
        associativity(
            detail::RawDrawing drawing) const {
            requireAnnotation();

            detail::RawBoolean associatedPosition = PRO_B_FALSE;
            detail::RawAnnotationAttachmentAssociativity
                attachment =
                    PRO_ANNOTATTACH_ASSOCIATIVITY_NONE;

            CREO_CHECK(
                detail::annotationIsAssociative(
                    &annotation_,
                    drawing,
                    &associatedPosition,
                    &attachment));

            ArchiCreoAnnotationAssociativity result{};
            result.position =
                associatedPosition == PRO_B_TRUE;
            result.attachment = attachment;
            return result;
        }

        detail::RawAnnotationElem element() const {
            requireAnnotation();

            detail::RawAnnotationElem result{};
            CREO_CHECK(
                detail::annotationElementGet(
                    &annotation_,
                    &result));

            return result;
        }

        void rotate(double rotation) const {
            requireAnnotation();

            CREO_CHECK(
                detail::annotationRotate(
                    &annotation_,
                    rotation));
        }

        [[nodiscard]] bool needsConversion() const {
            requireAnnotation();

            detail::RawBoolean conversionRequired =
                PRO_B_FALSE;

            CREO_CHECK(
                detail::annotationNeedsConversion(
                    &annotation_,
                    &conversionRequired));

            return conversionRequired == PRO_B_TRUE;
        }

        void convertLegacy() const {
            requireAnnotation();

            CREO_CHECK(
                detail::annotationLegacyConvert(
                    &annotation_));
        }

        void convertLegacyIfNeeded() const {
            if (!needsConversion()) {
                return;
            }

            convertLegacy();
        }

        void setDesignate(
            ::ProDesignateType designate) const {
            requireAnnotation();

            CREO_CHECK(
                detail::annotationDesignateSet(
                    &annotation_,
                    designate));
        }

        [[nodiscard]] ::ProDesignateType designate() const {
            requireAnnotation();

            ::ProDesignateType designate =
                PRO_DESIGNATE_NONE;

            CREO_CHECK(
                detail::annotationDesignateGet(
                    &annotation_,
                    &designate));

            return designate;
        }

        static void showByView(
            detail::RawDrawing drawing,
            detail::RawView view,
            detail::RawObjectType annotationType) {

            CREO_CHECK(
                detail::annotationByViewShow(
                    drawing,
                    view,
                    annotationType));
        }

        static void showByFeature(
            detail::RawDrawing drawing,
            detail::RawSelection feature,
            detail::RawView view,
            detail::RawObjectType annotationType) {

            CREO_CHECK(
                detail::annotationByFeatureShow(
                    drawing,
                    feature,
                    view,
                    annotationType));
        }

        static void showByComponent(
            detail::RawDrawing drawing,
            detail::RawSelection component,
            detail::RawView view,
            detail::RawObjectType annotationType) {

            CREO_CHECK(
                detail::annotationByComponentShow(
                    drawing,
                    component,
                    view,
                    annotationType));
        }

      private:
        void requireAnnotation() const {
            if (!valid_) {
                throw std::invalid_argument(
                    "ArchiCreoAnnotationHandler: "
                    "invalid annotation handle");
            }
        }

        detail::RawAnnotation annotation_;
        bool valid_;
    };

}
