#pragma once

/*
 * Example: C++17 hole-table creation/filling using the existing
 * ArchiCreoTableHandler / ArchiCreoCompat layers.
 *
 * This file deliberately does NOT change user_place_holeTable_().
 * The existing UserHoleProperties type and the existing hole-note helpers
 * remain application-owned and are supplied by the caller.
 *
 * The table-specific TOOLKIT calls stay behind ArchiCreoTableHandler and
 * ArchiCreoCompat. No ProTable/ProDwgtable API is called directly here.
 */

#include "ArchiCreoDrawingHandler.hpp"
#include "ArchiCreoErrorHandler.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace creo::example {

inline constexpr int kHoleTableColumns = 5;
inline constexpr double kHoleTableRowHeight = 7.0;

/*
 * Character-based widths used by ArchiCreoTableHandler.
 *
 * They preserve the intent of the legacy table:
 *   HOLE            : narrow
 *   DESCRIPTION     : widest
 *   QUANTITY        : narrow
 *   CHARACTERISTIC  : narrow
 *   POSITIONAL TOL  : medium
 *
 * The handler enforces its existing [5, 35] character limits.
 */
inline const std::vector<double> kHoleTableColumnWidths{
    5.0,
    35.0,
    8.0,
    14.0,
    18.0
};

inline ArchiCreoTableHandler createHoleTableExample(
    const ArchiCreoModelHandler& drawingModel,
    const detail::RawPoint3d& origin,
    int groupedHoleCount)
{
    if (!drawingModel.isValid()) {
        throw std::invalid_argument(
            "createHoleTableExample: invalid drawing model");
    }

    const detail::RawMdlType modelType =
        drawingModel.type();

    /*
     * Keep the historical behavior of accepting DRAWING and DWGFORM
     * at the caller level. The current ArchiCreoTableHandler::create()
     * itself is stricter and accepts PRO_MDL_DRAWING only.
     *
     * For an actual PRO_MDL_DWGFORM caller, the existing handler should
     * first be widened to accept that model type before this helper is used.
     */
    if (modelType != PRO_MDL_DRAWING) {
        throw std::invalid_argument(
            "createHoleTableExample: model is not a drawing");
    }

    if (groupedHoleCount <= 0) {
        throw std::invalid_argument(
            "createHoleTableExample: no grouped holes");
    }

    if (groupedHoleCount >
        detail::kMaxTableRows - 2) {
        throw std::invalid_argument(
            "createHoleTableExample: too many grouped holes");
    }

    const int tableRowCount =
        groupedHoleCount + 2;

    return ArchiCreoTableHandler::create(
        drawingModel,
        tableRowCount,
        kHoleTableColumns,
        origin,
        kHoleTableRowHeight,
        kHoleTableColumnWidths);
}

/*
 * Fills the complete HOLE TABLE body.
 *
 * HoleProperties is the application's existing UserHoleProperties type.
 * The callbacks keep the existing feature/annotation implementation out
 * of this example because those helpers are not part of this repository.
 *
 * nextLetter(holeIndex)            -> returns the same letter that the
 *                                    legacy userGetLastLetter() produced.
 * getMaximumDiameter(holeFeature)  -> same value as the legacy
 *                                    get_geomitem_cylinder_cone_max_diam.
 * placeHoleNote(holeFeature, letter, maxDiameter)
 *                                   -> same behavior as the legacy
 *                                      placeHoleNoteOnViews().
 *
 * The actual table creation/text/wrapping/auto-height is handled only by
 * ArchiCreoTableHandler.
 */
template <
    typename HoleProperties,
    typename NextLetter,
    typename GetMaximumDiameter,
    typename PlaceHoleNote>
void fillHoleTableExample(
    ArchiCreoTableHandler& table,
    HoleProperties** holeProperties,
    int groupedHoleCount,
    int* arrayGroupedHoles,
    NextLetter&& nextLetter,
    GetMaximumDiameter&& getMaximumDiameter,
    PlaceHoleNote&& placeHoleNote)
{
    if (!table.isValid()) {
        throw std::invalid_argument(
            "fillHoleTableExample: invalid table");
    }

    if (holeProperties == nullptr ||
        arrayGroupedHoles == nullptr) {
        throw std::invalid_argument(
            "fillHoleTableExample: invalid hole arrays");
    }

    if (groupedHoleCount <= 0) {
        return;
    }

    if (table.columnCount() != kHoleTableColumns ||
        table.rowCount() != groupedHoleCount + 2) {
        throw std::runtime_error(
            "fillHoleTableExample: unexpected table dimensions");
    }

    /*
     * The original table merged the five title cells after creation.
     * No repaint is requested: the final display happens once, after
     * the complete table has been populated.
     */
    table.mergeCells(
        1,
        1,
        kHoleTableColumns,
        1,
        false);

    std::vector<ArchiCreoTableCell> cells;
    cells.reserve(
        static_cast<std::size_t>(
            groupedHoleCount * kHoleTableColumns + 6));

    /*
     * Title.
     */
    cells.push_back({
        1,
        1,
        L"HOLE TABLE"
    });

    /*
     * Column headers.
     */
    cells.push_back({1, 2, L"HOLE"});
    cells.push_back({2, 2, L"DESCRIPTION"});
    cells.push_back({3, 2, L"QUANTITY"});
    cells.push_back({4, 2, L"CHARACTERISTIC"});
    cells.push_back({5, 2, L"POSITIONAL TOL"});

    /*
     * Body.
     *
     * The legacy content is preserved:
     *   1: generated hole letter
     *   2: noteText[0]
     *   3: quantity
     *   4: W_POS_SYMB
     *   5: W_DIAM_SYMB + "0.2"
     */
    for (int groupIndex = 0;
         groupIndex < groupedHoleCount;
         ++groupIndex) {

        const int propertyIndex =
            arrayGroupedHoles[groupIndex];

        if (propertyIndex < 0) {
            throw std::out_of_range(
                "fillHoleTableExample: negative grouped-hole index");
        }

        /*
         * UserHoleProperties remains owned by the application.
         * No allocation or deletion is performed here.
         */
        HoleProperties& hole =
            *holeProperties[propertyIndex];

        const std::wstring letter =
            nextLetter(groupIndex);

        if (letter.empty()) {
            throw std::runtime_error(
                "fillHoleTableExample: empty generated hole letter");
        }

        const std::wstring description =
            hole.noteText[0];

        const std::wstring quantity =
            std::to_wstring(hole.quantity);

        cells.push_back({
            1,
            groupIndex + 3,
            letter
        });

        cells.push_back({
            2,
            groupIndex + 3,
            description
        });

        cells.push_back({
            3,
            groupIndex + 3,
            quantity
        });

        cells.push_back({
            4,
            groupIndex + 3,
            W_POS_SYMB
        });

        cells.push_back({
            5,
            groupIndex + 3,
            std::wstring(W_DIAM_SYMB) + L"0.2"
        });
    }

    /*
     * One batched table operation:
     *   - validates cells
     *   - keeps column widths within the existing [5, 35] range
     *   - wraps text
     *   - enables row auto-height
     */
    table.setCells(cells);

    /*
     * Annotation/leader handling remains exactly where it was
     * conceptually in the legacy implementation.
     */
    for (int groupIndex = 0;
         groupIndex < groupedHoleCount;
         ++groupIndex) {

        const int propertyIndex =
            arrayGroupedHoles[groupIndex];

        if (propertyIndex < 0) {
            throw std::out_of_range(
                "fillHoleTableExample: negative grouped-hole index");
        }

        HoleProperties& leader =
            *holeProperties[propertyIndex];

        const std::wstring letter =
            nextLetter(groupIndex);

        const double maximumDiameter =
            getMaximumDiameter(
                leader.holeFeature);

        placeHoleNote(
            leader.holeFeature,
            letter,
            maximumDiameter);

        /*
         * Equivalent to the legacy wcscpy(), without unchecked C-string
         * operations. The exact destination type/size remains that of
         * the application's existing UserHoleProperties structure.
         */
        std::copy(
            letter.begin(),
            letter.end(),
            leader.associatedLetter);

        leader.associatedLetter[
            letter.size()] = L'\\0';

        /*
         * Preserve the legacy child-hole behavior:
         * children are found by matching leaderId and receive the same
         * leader letter and maximum diameter.
         */
        if (leader.quantity > 1) {
            int childrenLetterAdd = 1;
            int childOffset = 1;

            while (
                childrenLetterAdd <
                leader.quantity) {

                const int childIndex =
                    groupIndex + childOffset;

                if (childIndex < 0) {
                    throw std::runtime_error(
                        "fillHoleTableExample: invalid child index");
                }

                HoleProperties& child =
                    *holeProperties[childIndex];

                if (leader.holeFeature.id ==
                    child.leaderId) {

                    placeHoleNote(
                        child.holeFeature,
                        letter,
                        maximumDiameter);

                    ++childrenLetterAdd;
                }

                ++childOffset;
            }
        }
    }
}

/*
 * Example of how the existing user_place_holeTable_() can use this code
 * without changing its public signature:
 *
 *   1. Keep its existing ProError return type/signature.
 *   2. Keep ProArraySizeGet / the +2 header rows logic.
 *   3. Obtain the origin using the existing placement mechanism.
 *   4. Build ArchiCreoModelHandler from drw.
 *   5. Call createHoleTableExample().
 *   6. Call fillHoleTableExample().
 *   7. Call table.display() once, at the very end.
 *
 * user_place_holeTable_() itself is intentionally not redefined here.
 */

} // namespace creo::example
