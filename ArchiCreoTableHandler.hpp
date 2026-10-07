#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "ArchiCreoCompat.hpp"
#include "ArchiCreoErrorHandler.hpp"
#include "ArchiCreoModelHandler.hpp"
#include "ArchiScopeGuard.hpp"

namespace creo {

    struct ArchiCreoTableCell {
        int column;
        int row;
        std::wstring text;
    };

    class ArchiCreoTableHandler final {
      public:
        static constexpr int kMinColumnWidth = 5;
        static constexpr int kMaxColumnWidth = 35;
        static constexpr int kMaxRows = 100;
        static constexpr int kMaxColumns = 50;

        ArchiCreoTableHandler() noexcept :
            table_{},
            valid_(false) {
        }

        explicit ArchiCreoTableHandler(
            detail::RawTable table) noexcept :
            table_(table),
            valid_(true) {
        }

        [[nodiscard]] bool isValid() const noexcept {
            return valid_;
        }

        [[nodiscard]] detail::RawTable raw() const noexcept {
            return table_;
        }

        static ArchiCreoTableHandler create(
            const ArchiCreoModelHandler& model,
            int rows,
            int columns,
            const detail::RawPoint3d& origin,
            double rowHeight = 1.0,
            const std::vector<double>& columnWidths = {}) {

            if (!model.isValid()) {
                throw std::invalid_argument(
                    "ArchiCreoTableHandler::create: invalid model");
            }

            if (model.type() != PRO_MDL_DRAWING) {
                throw std::invalid_argument(
                    "ArchiCreoTableHandler::create: model is not a drawing");
            }

            validateDimensions(rows, columns);

            if (rowHeight <= 0.0) {
                throw std::invalid_argument(
                    "ArchiCreoTableHandler::create: "
                    "row height must be greater than zero");
            }

            const std::vector<double> widths =
                normalizeColumnWidths(columns, columnWidths);

            std::vector<double> rowHeights(
                static_cast<std::size_t>(rows),
                rowHeight);

            std::vector<ProHorzJust> justifications(
                static_cast<std::size_t>(columns),
                PRO_HORZ_JUSTIFY_LEFT);

            detail::RawTableData data = nullptr;

            CREO_CHECK(
                detail::tableDataAlloc(&data));

            CREO_CHECK(
                detail::tableDataOriginSet(
                    data,
                    origin));

            CREO_CHECK(
                detail::tableDataSizetypeSet(
                    data,
                    PROTABLESIZE_CHARACTERS));

            CREO_CHECK(
                detail::tableDataRowsSet(
                    data,
                    rows,
                    rowHeights.data()));

            CREO_CHECK(
                detail::tableDataColumnsSet(
                    data,
                    columns,
                    const_cast<double*>(widths.data()),
                    justifications.data()));

            detail::RawTable table{};

            CREO_CHECK(
                detail::tableCreate(
                    model.raw(),
                    data,
                    PRO_B_TRUE,
                    &table));

            return ArchiCreoTableHandler(table);
        }

        [[nodiscard]] int rowCount() const {
            requireTable();

            int rows = 0;
            int columns = 0;

            CREO_CHECK(
                detail::tableRowsColumnsCount(
                    &table_,
                    &rows,
                    &columns));

            return rows;
        }

        [[nodiscard]] int columnCount() const {
            requireTable();

            int rows = 0;
            int columns = 0;

            CREO_CHECK(
                detail::tableRowsColumnsCount(
                    &table_,
                    &rows,
                    &columns));

            return columns;
        }

        void setCellText(
            int column,
            int row,
            const std::wstring& text) const {

            requireCell(column, row);

            setColumnWidthForText(
                column,
                text);

            enterWrappedCellText(
                column,
                row,
                text);

            enableRowAutoHeight(row);
        }

        void setCells(
            const std::vector<ArchiCreoTableCell>& cells) const {

            requireTable();

            for (const ArchiCreoTableCell& cell : cells) {
                requireCell(
                    cell.column,
                    cell.row);
            }

            std::vector<double> requiredWidths(
                static_cast<std::size_t>(columnCount()),
                static_cast<double>(kMinColumnWidth));

            for (const ArchiCreoTableCell& cell : cells) {
                const std::size_t lineLength =
                    maxLineLength(cell.text);

                requiredWidths[
                    static_cast<std::size_t>(
                        cell.column - 1)] =
                    std::max(
                        requiredWidths[
                            static_cast<std::size_t>(
                                cell.column - 1)],
                        static_cast<double>(
                            std::clamp(
                                lineLength,
                                static_cast<std::size_t>(0),
                                static_cast<std::size_t>(
                                    kMaxColumnWidth))));
            }

            for (int column = 1;
                 column <= columnCount();
                 ++column) {

                setColumnWidth(
                    column,
                    requiredWidths[
                        static_cast<std::size_t>(
                            column - 1)]);
            }

            for (const ArchiCreoTableCell& cell : cells) {
                enterWrappedCellText(
                    cell.column,
                    cell.row,
                    cell.text);

                enableRowAutoHeight(
                    cell.row);
            }
        }

        void setColumnWidth(
            int column,
            double width) const {

            requireTable();

            if (column < 1 ||
                column > columnCount()) {
                throw std::out_of_range(
                    "ArchiCreoTableHandler::setColumnWidth: "
                    "column index out of range");
            }

            const double normalizedWidth =
                std::clamp(
                    width,
                    static_cast<double>(kMinColumnWidth),
                    static_cast<double>(kMaxColumnWidth));

            CREO_CHECK(
                detail::tableColumnWidthSet(
                    &table_,
                    column,
                    normalizedWidth,
                    PROTABLESIZE_CHARS_TRUE));
        }

        void mergeCells(
            int startColumn,
            int startRow,
            int endColumn,
            int endRow,
            bool display = true) const {

            requireCell(startColumn, startRow);
            requireCell(endColumn, endRow);

            if (startColumn > endColumn ||
                startRow > endRow) {
                throw std::invalid_argument(
                    "ArchiCreoTableHandler::mergeCells: "
                    "invalid cell range");
            }

            CREO_CHECK(
                detail::tableCellsMerge(
                    &table_,
                    startColumn,
                    startRow,
                    endColumn,
                    endRow,
                    display ? PRO_B_TRUE : PRO_B_FALSE));
        }

        void addRow(
            int insertAfterRow,
            double height = -1.0,
            bool display = true) const {

            requireTable();

            if (insertAfterRow > rowCount()) {
                throw std::out_of_range(
                    "ArchiCreoTableHandler::addRow: "
                    "row index out of range");
            }

            CREO_CHECK(
                detail::tableRowAdd(
                    &table_,
                    insertAfterRow,
                    display ? PRO_B_TRUE : PRO_B_FALSE,
                    height));
        }

        void addColumn(
            int insertAfterColumn,
            double width = -1.0,
            bool display = true) const {

            requireTable();

            if (insertAfterColumn > columnCount()) {
                throw std::out_of_range(
                    "ArchiCreoTableHandler::addColumn: "
                    "column index out of range");
            }

            const double normalizedWidth =
                width < 0.0
                    ? width
                    : std::clamp(
                          width,
                          static_cast<double>(kMinColumnWidth),
                          static_cast<double>(kMaxColumnWidth));

            CREO_CHECK(
                detail::tableColumnAdd(
                    &table_,
                    insertAfterColumn,
                    display ? PRO_B_TRUE : PRO_B_FALSE,
                    normalizedWidth));
        }

        void deleteRow(
            int row,
            bool display = true) const {

            requireCell(1, row);

            CREO_CHECK(
                detail::tableRowDelete(
                    &table_,
                    row,
                    display ? PRO_B_TRUE : PRO_B_FALSE));
        }

        void deleteColumn(
            int column,
            bool display = true) const {

            requireCell(column, 1);

            CREO_CHECK(
                detail::tableColumnDelete(
                    &table_,
                    column,
                    display ? PRO_B_TRUE : PRO_B_FALSE));
        }

      private:
        static void validateDimensions(
            int rows,
            int columns) {

            if (rows <= 0 || rows > kMaxRows) {
                throw std::invalid_argument(
                    "ArchiCreoTableHandler::create: "
                    "rows must be between 1 and 100");
            }

            if (columns <= 0 || columns > kMaxColumns) {
                throw std::invalid_argument(
                    "ArchiCreoTableHandler::create: "
                    "columns must be between 1 and 50");
            }
        }

        static std::vector<double> normalizeColumnWidths(
            int columns,
            const std::vector<double>& requestedWidths) {

            if (!requestedWidths.empty() &&
                requestedWidths.size() !=
                    static_cast<std::size_t>(columns)) {
                throw std::invalid_argument(
                    "ArchiCreoTableHandler::create: "
                    "columnWidths size does not match columns");
            }

            std::vector<double> widths(
                static_cast<std::size_t>(columns),
                static_cast<double>(kMinColumnWidth));

            for (int column = 0;
                 column < columns;
                 ++column) {

                if (!requestedWidths.empty()) {
                    widths[
                        static_cast<std::size_t>(column)] =
                        std::clamp(
                            requestedWidths[
                                static_cast<std::size_t>(column)],
                            static_cast<double>(kMinColumnWidth),
                            static_cast<double>(kMaxColumnWidth));
                }
            }

            return widths;
        }

        static std::size_t maxLineLength(
            const std::wstring& text) {

            std::size_t maxLength = 0;
            std::size_t currentLength = 0;

            for (const wchar_t character : text) {
                if (character == L'\n') {
                    maxLength = std::max(
                        maxLength,
                        currentLength);
                    currentLength = 0;
                    continue;
                }

                ++currentLength;
            }

            return std::max(
                maxLength,
                currentLength);
        }

        void setColumnWidthForText(
            int column,
            const std::wstring& text) const {

            const double requiredWidth =
                static_cast<double>(
                    std::clamp(
                        maxLineLength(text),
                        static_cast<std::size_t>(kMinColumnWidth),
                        static_cast<std::size_t>(
                            kMaxColumnWidth)));

            double currentWidth =
                static_cast<double>(kMinColumnWidth);

            const detail::ProErrorCode status =
                detail::tableColumnWidthGet(
                    &table_,
                    column,
                    PROTABLESIZE_CHARS_TRUE,
                    &currentWidth);

            if (status != detail::kNoError) {
                CREO_CHECK(status);
            }

            setColumnWidth(
                column,
                std::max(
                    currentWidth,
                    requiredWidth));
        }

        void enterWrappedCellText(
            int column,
            int row,
            const std::wstring& text) const {

            const std::wstring normalized =
                normalizeLineBreaks(text);

            const std::size_t width =
                static_cast<std::size_t>(
                    getColumnWidthInCharacters(column));

            const std::vector<std::wstring> lines =
                wrapText(normalized, width);

            detail::RawWstringProArray textLines = nullptr;

            CREO_CHECK(
                detail::wstringProArrayAlloc(
                    static_cast<int>(lines.size()),
                    &textLines));

            auto freeTextLines = Defer([&] {
                CREO_CHECK(
                    detail::wstringProArrayFree(
                        &textLines));
            });

            std::vector<std::vector<wchar_t>> buffers;
            buffers.reserve(lines.size());

            for (const std::wstring& line : lines) {
                buffers.emplace_back(
                    line.begin(),
                    line.end());

                buffers.back().push_back(L'\\0');
                textLines[buffers.size() - 1] =
                    buffers.back().data();
            }

            CREO_CHECK(
                detail::tableTextEnter(
                    &table_,
                    column,
                    row,
                    textLines));
        }

        double getColumnWidthInCharacters(
            int column) const {

            double width =
                static_cast<double>(kMinColumnWidth);

            CREO_CHECK(
                detail::tableColumnWidthGet(
                    &table_,
                    column,
                    PROTABLESIZE_CHARACTERS,
                    &width));

            return std::clamp(
                width,
                static_cast<double>(kMinColumnWidth),
                static_cast<double>(kMaxColumnWidth));
        }

        static std::wstring normalizeLineBreaks(
            const std::wstring& text) {

            std::wstring result;
            result.reserve(text.size());

            for (std::size_t i = 0; i < text.size(); ++i) {
                if (text[i] == L'\\r') {
                    if (i + 1 < text.size() &&
                        text[i + 1] == L'\\n') {
                        ++i;
                    }

                    result.push_back(L'\\n');
                } else {
                    result.push_back(text[i]);
                }
            }

            return result;
        }

        static std::vector<std::wstring> wrapText(
            const std::wstring& text,
            std::size_t width) {

            width = std::max<std::size_t>(
                width,
                static_cast<std::size_t>(kMinColumnWidth));

            std::vector<std::wstring> result;
            std::wstring currentLine;

            auto flushLine = [&] {
                result.push_back(currentLine);
                currentLine.clear();
            };

            std::size_t position = 0;

            while (position <= text.size()) {
                const std::size_t lineEnd =
                    text.find(L'\\n', position);

                const std::wstring paragraph =
                    text.substr(
                        position,
                        lineEnd == std::wstring::npos
                            ? std::wstring::npos
                            : lineEnd - position);

                if (paragraph.empty()) {
                    flushLine();
                } else {
                    std::size_t wordStart = 0;

                    while (wordStart < paragraph.size()) {
                        while (wordStart < paragraph.size() &&
                               paragraph[wordStart] == L' ') {
                            ++wordStart;
                        }

                        if (wordStart >= paragraph.size()) {
                            break;
                        }

                        std::size_t wordEnd =
                            paragraph.find(L' ', wordStart);

                        if (wordEnd == std::wstring::npos) {
                            wordEnd = paragraph.size();
                        }

                        const std::wstring word =
                            paragraph.substr(
                                wordStart,
                                wordEnd - wordStart);

                        if (word.size() > width) {
                            if (!currentLine.empty()) {
                                flushLine();
                            }

                            std::size_t offset = 0;
                            while (offset < word.size()) {
                                const std::size_t count =
                                    std::min(
                                        width,
                                        word.size() - offset);

                                result.push_back(
                                    word.substr(
                                        offset,
                                        count));

                                offset += count;
                            }
                        } else if (currentLine.empty()) {
                            currentLine = word;
                        } else if (currentLine.size() + 1 + word.size() <= width) {
                            currentLine += L' ';
                            currentLine += word;
                        } else {
                            flushLine();
                            currentLine = word;
                        }

                        wordStart =
                            wordEnd == paragraph.size()
                                ? paragraph.size()
                                : wordEnd + 1;
                    }

                    if (!currentLine.empty()) {
                        flushLine();
                    }
                }

                if (lineEnd == std::wstring::npos) {
                    break;
                }

                position = lineEnd + 1;
            }

            if (result.empty()) {
                result.emplace_back();
            }

            return result;
        }

        void enableRowAutoHeight(
            int row) const {

            CREO_CHECK(
                detail::tableRowHeightAutoAdjustSet(
                    &table_,
                    row,
                    PROTBLROWHEIGHT_AUTOADJUST_TRUE));
        }

        void requireTable() const {
            if (!valid_) {
                throw std::invalid_argument(
                    "ArchiCreoTableHandler: "
                    "invalid table handle");
            }
        }

        void requireCell(
            int column,
            int row) const {

            requireTable();

            if (column < 1 ||
                column > columnCount()) {
                throw std::out_of_range(
                    "ArchiCreoTableHandler: "
                    "column index out of range");
            }

            if (row < 1 ||
                row > rowCount()) {
                throw std::out_of_range(
                    "ArchiCreoTableHandler: "
                    "row index out of range");
            }
        }

        mutable detail::RawTable table_;
        bool valid_;
    };

}
