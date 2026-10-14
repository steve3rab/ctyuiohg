/*
 * Example: C++17 hole-table creation/filling using the existing
 * ArchiCreoTableHandler / ArchiCreoCompat layers.
 *
 * This file deliberately does NOT change user_place_holeTable_().
 * The existing UserHoleProperties type and existing hole-note helpers
 * remain application-owned.
 *
 * No ProTable/ProDwgtable API is called directly from this example.
 */

#include "ArchiCreoDrawingHandler.hpp"
#include "ArchiCreoErrorHandler.hpp"
#include "ArchiCreoTableHandler.hpp"
#include "ArchiCreoCompat.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

/*
 * Include here the existing project header that declares:
 *   - UserHoleProperties
 *   - userGetLastLetter()
 *   - get_geomitem_cylinder_cone_max_diam()
 *   - placeHoleNoteOnViews()
 *
 * That header is intentionally not invented here because it is not present
 * in the current repository.
 */

namespace creo::example {

namespace {

constexpr int kHoleTableColumns = 5;
constexpr double kHoleTableRowHeight = 7.0;

/*
 * Character-based widths used by ArchiCreoTableHandler.
 *
 * The handler already enforces its [5, 35] character limits.
 * These values keep the legacy layout intent:
 *   HOLE            : narrow
 *   DESCRIPTION     : widest
 *   QUANTITY        : narrow
 *   CHARACTERISTIC  : medium
 *   POSITIONAL TOL  : medium
 */
const std::vector<double> kHoleTableColumnWidths{
    5.0,
    35.0,
    8.0,
    14.0,
    18.0
};

} // namespace

creo::ArchiCreoTableHandler createHoleTableExample(
    const creo::ArchiCreoModelHandler& drawingModel,
    const detail::RawPoint3d& origin,
    int groupedHoleCount)
{
    if (!drawingModel.isValid()) {
        throw std::invalid_argument(
            "createHoleTableExample: invalid drawing model");
    }

    if (drawingModel.type() != PRO_MDL_DRAWING) {
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

    return creo::ArchiCreoTableHandler::create(
        drawingModel,
        tableRowCount,
        kHoleTableColumns,
        origin,
        kHoleTableRowHeight,
        kHoleTableColumnWidths);
}

/*
 * This implementation assumes the project's existing UserHoleProperties
 * and hole-note functions are visible from the calling translation unit.
 *
 * The function preserves the legacy table content:
 *   row 1: merged "HOLE TABLE"
 *   row 2: column headers
 *   row 3+: generated letter / description / quantity /
 *           characteristic / positional tolerance
 *
 * The table is displayed only once, after all cells are written.
 */
void fillHoleTableExample(
    creo::ArchiCreoTableHandler& table,
    UserHoleProperties** p_arrayHoleProperties,
    int groupedHoleCount,
    ProDrawing drawing,
    int* arrayGroupedHoles)
{
    if (!table.isValid()) {
        throw std::invalid_argument(
            "fillHoleTableExample: invalid table");
    }

    if (p_arrayHoleProperties == nullptr ||
        arrayGroupedHoles == nullptr ||
        drawing == nullptr) {
        throw std::invalid_argument(
            "fillHoleTableExample: invalid hole data");
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
     * Keep title merged, but do not display/repaint while building.
     */
    table.mergeCells(
        1,
        1,
        kHoleTableColumns,
        1,
        false);

    std::vector<creo::ArchiCreoTableCell> cells;
    cells.reserve(
        static_cast<std::size_t>(
            groupedHoleCount * kHoleTableColumns + 6));

    cells.push_back({
        1,
        1,
        L"HOLE TABLE"
    });

    cells.push_back({1, 2, L"HOLE"});
    cells.push_back({2, 2, L"DESCRIPTION"});
    cells.push_back({3, 2, L"QUANTITY"});
    cells.push_back({4, 2, L"CHARACTERISTIC"});
    cells.push_back({5, 2, L"POSITIONAL TOL"});

    /*
     * The original UserHoleProperties array remains application-owned.
     * This example only reads/writes its existing fields.
     */
    for (int groupIndex = 0;
         groupIndex < groupedHoleCount;
         ++groupIndex) {

        const int propertyIndex =
            arrayGroupedHoles[groupIndex];

        if (propertyIndex < 0) {
            throw std::out_of_range(
                "fillHoleTableExample: invalid grouped-hole index");
        }

        UserHoleProperties& hole =
            *p_arrayHoleProperties[propertyIndex];

        char letterSeed[3] = {
            ' ',
            '@',
            '\0'
        };

        wchar_t returnedLetter[3] = {};

        /*
         * Reproduce the existing letter generation logic.
         */
        for (int letterIndex = 0;
             letterIndex <= groupIndex;
             ++letterIndex) {
            userGetLastLetter(
                letterSeed,
                returnedLetter);
        }

        const int row =
            groupIndex + 3;

        cells.push_back({
            1,
            row,
            returnedLetter
        });

        cells.push_back({
            2,
            row,
            hole.noteText[0]
        });

        cells.push_back({
            3,
            row,
            std::to_wstring(hole.quantity)
        });

        cells.push_back({
            4,
            row,
            W_POS_SYMB
        });

        cells.push_back({
            5,
            row,
            std::wstring(W_DIAM_SYMB) + L"0.2"
        });
    }

    /*
     * Existing handler performs:
     *   - cell validation
     *   - width normalization [5, 35]
     *   - text wrapping
     *   - temporary ProWstring array allocation/free
     *   - row auto-height
     */
    table.setCells(cells);

    /*
     * Preserve the legacy hole-note processing.
     */
    for (int groupIndex = 0;
         groupIndex < groupedHoleCount;
         ++groupIndex) {

        const int propertyIndex =
            arrayGroupedHoles[groupIndex];

        if (propertyIndex < 0) {
            throw std::out_of_range(
                "fillHoleTableExample: invalid grouped-hole index");
        }

        UserHoleProperties& leader =
            *p_arrayHoleProperties[propertyIndex];

        char letterSeed[3] = {
            ' ',
            '@',
            '\0'
        };

        wchar_t returnedLetter[3] = {};

        for (int letterIndex = 0;
             letterIndex <= groupIndex;
             ++letterIndex) {
            userGetLastLetter(
                letterSeed,
                returnedLetter);
        }

        double threadMaxDiameter = -1.0;

        CREO_CHECK(
            detail::featureGeomitemVisit(
                &leader.holeFeature,
                PRO_SURFACE,
                get_geomitem_cylinder_cone_max_diam,
                PRO_NO_FUNCTION,
                &threadMaxDiameter));

        placeHoleNoteOnViews(
            &leader.holeFeature,
            drawing,
            returnedLetter,
            threadMaxDiameter);

        /*
         * No wcscpy(): bounded copy into the existing application buffer.
         *
         * The existing UserHoleProperties::associatedLetter buffer is
         * expected to have room for the legacy 3-character value.
         */
        std::copy(
            returnedLetter,
            returnedLetter + 3,
            leader.associatedLetter);

        /*
         * Keep the legacy child-hole behavior:
         * children are identified by leaderId and receive the same
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

                UserHoleProperties& child =
                    *p_arrayHoleProperties[childIndex];

                if (leader.holeFeature.id ==
                    child.leaderId) {

                    placeHoleNoteOnViews(
                        &child.holeFeature,
                        drawing,
                        returnedLetter,
                        threadMaxDiameter);

                    ++childrenLetterAdd;
                }

                ++childOffset;
            }
        }
    }

    /*
     * No explicit display here so the caller can choose when to repaint.
     * Recommended:
     *
     *     table.display();
     *
     * exactly once after fillHoleTableExample() succeeds.
     */
}

/*
 * Integration pattern for the existing user_place_holeTable_():
 *
 *   - keep its signature and all callers unchanged;
 *   - keep ProArraySizeGet() and the +2 header-row calculation;
 *   - keep its existing origin acquisition;
 *   - create ArchiCreoModelHandler from the existing drawing;
 *   - call createHoleTableExample();
 *   - call fillHoleTableExample();
 *   - call table.display() once at the end.
 *
 * The existing user_place_holeTable_() is intentionally NOT redefined here.
 */

} // namespace creo::example
