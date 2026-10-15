#pragma once

#include <ProToolkit.h>

#include <ProArray.h>
#include <ProAsmcomp.h>
#include <ProAsmcomppath.h>
#include <ProAnnotation.h>
#include <ProDrawing.h>
#include <ProEcadUtils.h>
#include <ProAssembly.h>
#include <ProCore.h>
#include <ProMdl.h>
#include <ProObjects.h>
#include <ProModelitem.h>
#include <ProIntfimport.h>
#include <ProSelection.h>
#include <ProView.h>
#include <ProParameter.h>
#include <ProParamval.h>
#include <ProSolid.h>
#if defined(CREO10) && defined(CREO13)
#error "Define only one of CREO10 or CREO13 when building ArchiCreo"
#elif defined(CREO10)
#include <ProDwgtable.h>
#elif defined(CREO13)
#include <ProTable.h>
#else
#error "Define exactly one of CREO10 or CREO13 when building ArchiCreo"
#endif
#include <ProUtil.h>
#include <ProWindows.h>

namespace creo::detail {

    using ProErrorCode = ::ProError;
    using RawMdl = ::ProMdl;
    using RawPath = ::ProPath;
    using RawArray = ::ProArray;
    using RawBoolean = ::ProBoolean;
    using RawModelItem = ::ProModelitem;
    using RawParameter = ::ProParameter;
    using RawParamValue = ::ProParamvalue;
    using RawParamValueType = ::ProParamvalueType;
    using RawLockStatus = ::ProLockstatus;
    using RawMdlType = ::ProMdlType;
    using RawMdlName = ::ProMdlName;
    using RawName = ::ProName;
    using RawMdlFileType = ::ProMdlfileType;
    using RawMdlSubtype = ::ProMdlsubtype;
    using RawMdlExtension = ::ProMdlExtension;
    using RawMdlFileName = ::ProMdlFileName;
    using RawLine = ::ProLine;
    using RawFileListOpt = ::ProFileListOpt;
    using RawObjectType = ::ProType;
    using RawSolid = ::ProSolid;
    using RawAssembly = ::ProAssembly;
    using RawAsmcomp = ::ProAsmcomp;
    using RawMatrix = ::ProMatrix;
    using RawVector = ::ProVector;
    using RawPoint3d = ::ProPoint3d;
    using RawDrawing = ::ProDrawing;
    using RawAnnotation = ::ProAnnotation;
    using RawAnnotationElem = ::ProAnnotationElem;
    using RawView = ::ProView;
    using RawSelection = ::ProSelection;
    using RawAsmcompPath = ::ProAsmcomppath;
    using RawAnnotationAttachmentAssociativity =
        ::ProAnnotationAttachmentAssociativity;
    using RawIntfImportType = ::ProIntfImportType;
    using RawImportRepType = ::ProImportRepType;
    using RawIntfImportLayerFilter = ::ProIntfimportLayerFilter;
    using RawAppData = ::ProAppData;
    using RawSolidDispCompVisitAction = ::ProSolidDispCompVisitAction;
    using RawSolidDispCompFilterAction = ::ProSolidDispCompFilterAction;
    using RawEcadType = ::ProTKEcadType;
    using RawEcadImpOpts = ::ProEcadImpOpts;
#if defined(CREO10)
    using RawTable = ::ProDwgtable;
    using RawTableData = ::ProDwgtabledata;
    using RawTableSizeType = ::ProDwgtableSizetype;
    using RawTableRowHeightAutoadjustType = ::ProDwgtableRowheightAutoadjusttype;
    using RawTableParamMode = ::ProParamMode;
#elif defined(CREO13)
    using RawTable = ::ProTable;
    using RawTableData = ::ProTableData;
    using RawTableSizeType = ::ProTableSizetype;
    using RawTableRowHeightAutoadjustType = ::ProTableRowheightAutoadjusttype;
    using RawTableParamMode = ::ProTableParamMode;
#endif
    using RawWstringProArray = ::ProWstring*;

#if defined(CREO10)
    inline constexpr RawObjectType kTableObjectType =
        PRO_DRAW_TABLE;
    inline constexpr RawTableSizeType kTableSizeCharacters =
        PRODWGTABLESIZE_CHARACTERS;
    inline constexpr RawTableSizeType kTableSizeCharsTrue =
        PRODWGTABLESIZE_CHARS_TRUE;
    inline constexpr RawTableRowHeightAutoadjustType
        kTableRowHeightAutoAdjustTrue =
            PRODWGTBLROWHEIGHT_AUTOADJUST_TRUE;
#elif defined(CREO13)
    inline constexpr RawObjectType kTableObjectType =
        PRO_TABLE;
    inline constexpr RawTableSizeType kTableSizeCharacters =
        PROTABLESIZE_CHARACTERS;
    inline constexpr RawTableSizeType kTableSizeCharsTrue =
        PROTABLESIZE_CHARS_TRUE;
    inline constexpr RawTableRowHeightAutoadjustType
        kTableRowHeightAutoAdjustTrue =
            PROTBLROWHEIGHT_AUTOADJUST_TRUE;
#endif

    inline constexpr int kMaxTableColumns =
        PRO_TABLE_MAX_COLS;
    inline constexpr int kMaxTableRows =
        PRO_TABLE_MAX_ROWS;

    inline constexpr RawBoolean kBooleanFalse = PRO_B_FALSE;
    inline constexpr RawBoolean kBooleanTrue = PRO_B_TRUE;
    inline constexpr ProErrorCode kNoError = PRO_TK_NO_ERROR;

    inline ProErrorCode arrayAlloc(
        int n_objs,
        int obj_size,
        int reallocation_size,
        RawArray* p_array) {
        return ::ProArrayAlloc(
            n_objs, obj_size, reallocation_size, p_array);
    }

    inline ProErrorCode arrayFree(RawArray* p_array) {
        if (p_array == nullptr || *p_array == nullptr) {
            return kNoError;
        }

        const ProErrorCode status =
            ::ProArrayFree(p_array);

        if (status == kNoError) {
            *p_array = nullptr;
        }

        return status;
    }

    inline ProErrorCode arraySizeGet(RawArray array, int* p_size) {
        return ::ProArraySizeGet(array, p_size);
    }

    inline ProErrorCode arrayMaxCountGet(
        int obj_size,
        int* max_num_objs) {
        return ::ProArrayMaxCountGet(obj_size, max_num_objs);
    }

    inline ProErrorCode mdlMdlNameGet(
        RawMdl model,
        wchar_t* name_out) {
        return ::ProMdlMdlnameGet(model, name_out);
    }

    inline ProErrorCode mdlToModelitem(
        RawMdl model,
        RawModelItem* p_item) {
        return ::ProMdlToModelitem(model, p_item);
    }

    inline ProErrorCode paramvalueSet(
        RawParamValue* p_handle,
        void* value,
        RawParamValueType type) {
        return ::ProParamvalueSet(
            p_handle,
            value,
            type);
    }

    inline ProErrorCode paramvalueValueGet(
        RawParamValue* p_handle,
        RawParamValueType type,
        void* p_value) {
        return ::ProParamvalueValueGet(
            p_handle,
            type,
            p_value);
    }

    inline ProErrorCode paramvalueTypeGet(
        RawParamValue* p_handle,
        RawParamValueType* p_type) {
        return ::ProParamvalueTypeGet(p_handle, p_type);
    }

    inline ProErrorCode parameterInit(
        RawModelItem* owner,
        RawName name,
        RawParameter* p_param) {
        return ::ProParameterInit(owner, name, p_param);
    }

    inline ProErrorCode parameterCreate(
        RawModelItem* owner,
        RawName name,
        RawParamValue* value,
        RawParameter* p_param) {
        return ::ProParameterWithUnitsCreate(
            owner, name, value, nullptr, p_param);
    }

    inline ProErrorCode parameterValueGet(
        RawParameter* param,
        RawParamValue* p_value) {
        return ::ProParameterValueWithUnitsGet(
            param, p_value, nullptr);
    }

    inline ProErrorCode parameterValueSet(
        RawParameter* param,
        RawParamValue* value) {
        return ::ProParameterValueWithUnitsSet(
            param, value, nullptr);
    }

    inline ProErrorCode parameterDelete(RawParameter* param) {
        return ::ProParameterDelete(param);
    }

    inline ProErrorCode parameterLockstatusGet(
        RawParameter* param,
        RawLockStatus* p_status) {
        return ::ProParameterLockstatusGet(param, p_status);
    }

    inline ProErrorCode mdlCurrentGet(RawMdl* p_mdl) {
        return ::ProMdlCurrentGet(p_mdl);
    }

    inline ProErrorCode mdlActiveGet(RawMdl* p_mdl) {
        return ::ProMdlActiveGet(p_mdl);
    }

    inline ProErrorCode mdlTypeGet(
        RawMdl model,
        RawMdlType* p_type) {
        return ::ProMdlTypeGet(model, p_type);
    }

    inline ProErrorCode mdlFiletypeGet(
        RawMdl model,
        RawMdlFileType* p_type) {
        return ::ProMdlFiletypeGet(model, p_type);
    }

    inline ProErrorCode modelitemNameGet(
        RawModelItem* item,
        wchar_t* name_out) {
        return ::ProModelitemNameGet(item, name_out);
    }

    inline ProErrorCode modelitemMdlGet(
        RawModelItem* item,
        RawMdl* p_model) {
        return ::ProModelitemMdlGet(item, p_model);
    }

    inline ProErrorCode mdlExtensionGet(
        RawMdl model,
        wchar_t* ext_out) {
        return ::ProMdlExtensionGet(model, ext_out);
    }

    inline ProErrorCode mdlDirectoryPathGet(
        RawMdl model,
        wchar_t* dir_path_out) {
        return ::ProMdlDirectoryPathGet(model, dir_path_out);
    }

    inline ProErrorCode mdlDisplay(RawMdl model) {
        return ::ProMdlDisplay(model);
    }

    inline ProErrorCode treetoolRefresh(RawMdl model) {
        return ::ProTreetoolRefresh(model);
    }

    inline ProErrorCode asmcompAssemble(
        RawAssembly assembly,
        RawSolid component_model,
        RawMatrix init_position,
        RawAsmcomp* p_feature) {
        return ::ProAsmcompAssemble(
            assembly,
            component_model,
            init_position,
            p_feature);
    }

    inline ProErrorCode matrixInit(
        RawVector x_vector,
        RawVector y_vector,
        RawVector z_vector,
        RawPoint3d origin,
        RawMatrix matrix) {
        return ::ProMatrixInit(
            x_vector,
            y_vector,
            z_vector,
            origin,
            matrix);
    }

    inline ProErrorCode sessionModelList(
        RawMdlType model_type,
        RawMdl** p_model_array,
        int* p_count) {
        return ::ProSessionMdlList(
            model_type,
            p_model_array,
            p_count);
    }

    inline ProErrorCode sessionModelListFree(
        RawMdl** p_model_array) {
        if (p_model_array == nullptr ||
            *p_model_array == nullptr) {
            return kNoError;
        }

        const ProErrorCode status =
            ::ProArrayFree(
                reinterpret_cast<RawArray*>(
                    p_model_array));

        if (status == kNoError) {
            *p_model_array = nullptr;
        }

        return status;
    }

    inline ProErrorCode mdlWindowGet(
        RawMdl model,
        int* window_id) {
        return ::ProMdlWindowGet(model, window_id);
    }

    inline ProErrorCode mdlSave(RawMdl model) {
        return ::ProMdlSave(model);
    }

    inline ProErrorCode mdlErase(RawMdl model) {
        return ::ProMdlErase(model);
    }

    inline ProErrorCode mdlIsSaveAllowed(
        RawMdl model,
        RawBoolean show_ui,
        RawBoolean* save_allowed) {
        return ::ProMdlIsSaveAllowed(
            model, show_ui, save_allowed);
    }

    inline ProErrorCode mdlnameRename(
        RawMdl model,
        wchar_t* new_name) {
        return ::ProMdlnameRename(model, new_name);
    }

    inline ProErrorCode mdlModificationVerify(
        RawMdl model,
        RawBoolean* p_modified) {
        return ::ProMdlModificationVerify(
            model, p_modified);
    }

    inline ProErrorCode mdlFiletypeLoad(
        RawPath full_path,
        RawMdlFileType type,
        RawBoolean ask_user_about_reps,
        RawMdl* p_handle) {
        return ::ProMdlFiletypeLoad(
            full_path,
            type,
            ask_user_about_reps,
            p_handle);
    }

    inline ProErrorCode mdlnameCopy(
        RawMdl model,
        RawMdlName new_name,
        RawMdl* p_new_handle) {
        return ::ProMdlnameCopy(
            model,
            new_name,
            p_new_handle);
    }

    inline ProErrorCode solidMdlnameCreate(
        RawMdlName name,
        RawMdlFileType type,
        RawSolid* p_handle) {
        return ::ProSolidMdlnameCreate(
            name, type, p_handle);
    }

    inline ProErrorCode solidRegenerate(
        RawSolid solid,
        int flags) {
        return ::ProSolidRegenerate(solid, flags);
    }

    inline ProErrorCode solidDispCompVisit(
        RawSolid handle,
        RawSolidDispCompVisitAction visit_action,
        RawSolidDispCompFilterAction filter_action,
        RawAppData app_data) {
        return ::ProSolidDispCompVisit(
            handle,
            visit_action,
            filter_action,
            app_data);
    }

    inline RawSolid mdlToSolid(RawMdl model) {
        return static_cast<RawSolid>(model);
    }

    inline ProErrorCode stringFree(char** p_string) {
        if (p_string == nullptr || *p_string == nullptr) {
            return kNoError;
        }

        const ProErrorCode status =
            ::ProStringFree(*p_string);

        if (status == kNoError) {
            *p_string = nullptr;
        }

        return status;
    }

    inline ProErrorCode windowCurrentGet(int* p_window_id) {
        return ::ProWindowCurrentGet(p_window_id);
    }

    inline ProErrorCode windowCurrentSet(int window_id) {
        return ::ProWindowCurrentSet(window_id);
    }

    inline ProErrorCode windowMdlGet(
        int window_id,
        RawMdl* p_mdl) {
        return ::ProWindowMdlGet(window_id, p_mdl);
    }

    inline ProErrorCode windowNameGet(
        int window_id,
        char** p_window_name) {
        return ::ProWindowNameGet(
            window_id, p_window_name);
    }

    inline ProErrorCode windowRefresh(int window_id) {
        return ::ProWindowRefresh(window_id);
    }

    inline ProErrorCode windowRepaint(int window_id) {
        return ::ProWindowRepaint(window_id);
    }

    inline ProErrorCode windowActivate(int window_id) {
        return ::ProWindowActivate(window_id);
    }

    inline ProErrorCode objectwindowMdlnameCreate(
        RawMdlName object_name,
        RawObjectType object_type,
        int* p_window_id) {
        return ::ProObjectwindowMdlnameCreate(
            object_name, object_type, p_window_id);
    }

    inline ProErrorCode configoptionGet(
        wchar_t* option,
        wchar_t* option_value) {
        return ::ProConfigoptionGet(
            option, option_value);
    }

    inline ProErrorCode configoptSet(
        wchar_t* option,
        wchar_t* option_value) {
        return ::ProConfigoptSet(
            option, option_value);
    }

    inline ProErrorCode directoryChange(
        RawPath path) {
        return ::ProDirectoryChange(path);
    }

    inline ProErrorCode directoryCurrentGet(
        RawPath path) {
        return ::ProDirectoryCurrentGet(path);
    }

    inline ProErrorCode filesList(
        RawPath directory_path,
        RawLine filter,
        RawFileListOpt listing_option,
        RawPath** p_file_name_array,
        RawPath** p_subdir_name_array) {
        return ::ProFilesList(
            directory_path,
            filter,
            listing_option,
            p_file_name_array,
            p_subdir_name_array);
    }

    inline ProErrorCode directoryChoose(
        RawName dialog_label,
        RawPath* shortcut_path_arr,
        RawName* shortcut_name_arr,
        RawPath default_path,
        RawPath selected_path) {
        return ::ProDirectoryChoose(
            dialog_label,
            shortcut_path_arr,
            shortcut_name_arr,
            default_path,
            selected_path);
    }

    inline ProErrorCode fileMdlnameOpen(
        RawName dialog_label,
        RawLine filter_string,
        RawPath* shortcut_path_arr,
        RawName* shortcut_name_arr,
        RawPath default_path,
        RawMdlFileName pre_sel_file_name,
        RawPath selected_file) {
        return ::ProFileMdlnameOpen(
            dialog_label,
            filter_string,
            shortcut_path_arr,
            shortcut_name_arr,
            default_path,
            pre_sel_file_name,
            selected_file);
    }

    inline ProErrorCode fileMdlnameSave(
        RawName dialog_label,
        RawLine filter_string,
        RawPath* shortcut_path_arr,
        RawName* shortcut_name_arr,
        RawPath default_path,
        RawMdlFileName pre_sel_file_name,
        RawPath selected_file) {
        return ::ProFileMdlnameSave(
            dialog_label,
            filter_string,
            shortcut_path_arr,
            shortcut_name_arr,
            default_path,
            pre_sel_file_name,
            selected_file);
    }

    inline ProErrorCode fileMdlnameParse(
        RawPath file_name_w_path,
        RawPath r_path,
        RawMdlName r_file_name,
        RawMdlExtension r_extension,
        int* r_version) {
        return ::ProFileMdlnameParse(
            file_name_w_path,
            r_path,
            r_file_name,
            r_extension,
            r_version);
    }

    inline ProErrorCode pathMdlnameCreate(
        RawPath path,
        RawMdlName file_name,
        RawMdlExtension extension,
        int version,
        RawPath result) {
        return ::ProPathMdlnameCreate(
            path,
            file_name,
            extension,
            version,
            result);
    }

    inline ProErrorCode fileSubtypeGet(
        RawPath file,
        RawMdlFileType* file_type,
        RawMdlType* model_type,
        RawMdlSubtype* subtype) {
        return ::ProFileSubtypeGet(
            file,
            file_type,
            model_type,
            subtype);
    }


    inline ProErrorCode intfImportModelWithOptionsMdlnameCreate(
        RawPath import_file,
        RawPath profile,
        RawIntfImportType type,
        RawMdlType create_type,
        RawImportRepType rep_type,
        RawMdlName new_model_name,
        ::ProIntfimportLayerFilter filter_func,
        ::ProAppData application_data,
        RawMdl* created_model) {
        return ::ProIntfimportModelWithOptionsMdlnameCreate(
            import_file,
            profile,
            type,
            create_type,
            rep_type,
            new_model_name,
            filter_func,
            application_data,
            created_model);
    }

    inline ProErrorCode ecadImport(
        RawPath filename,
        RawEcadImpOpts options,
        RawEcadType ecad_type,
        RawMdl* new_model) {
        return ::ProEcadImport(
            filename,
            options,
            ecad_type,
            new_model);
    }

    inline ProErrorCode ecadImpOptsAlloc(
        RawEcadImpOpts* p_options) {
        return ::ProEcadImpOptsAlloc(
            p_options);
    }

    inline ProErrorCode ecadImpOptsFree(
        RawEcadImpOpts* p_options) {
        if (p_options == nullptr ||
            *p_options == nullptr) {
            return kNoError;
        }

        const ProErrorCode status =
            ::ProEcadImpOptsFree(p_options);

        if (status == kNoError) {
            *p_options = nullptr;
        }

        return status;
    }

    inline ProErrorCode ecadImpOptsFlexDesignSet(
        RawEcadImpOpts options,
        RawBoolean flex_design) {
        return ::ProEcadImpOptsFlexDesignSet(
            options,
            flex_design);
    }

    inline ProErrorCode ecadImpOptsIDF3EmpFileNameSet(
        RawEcadImpOpts options,
        RawPath idf_emp_filename) {
        return ::ProEcadImpOptsIDF3EmpFileNameSet(
            options,
            idf_emp_filename);
    }

    inline ProErrorCode ecadImpOptsIDF3ImpOtherOutlineSet(
        RawEcadImpOpts options,
        RawBoolean import_other_outline) {
        return ::ProEcadImpOptsIDF3ImpOtherOutlineSet(
            options,
            import_other_outline);
    }

#if defined(CREO10)

    inline ProErrorCode tableDataAlloc(RawTableData* p_data) {
        return ::ProDwgtabledataAlloc(p_data);
    }

    inline ProErrorCode tableDataOriginSet(
        RawTableData data, RawPoint3d origin) {
        return ::ProDwgtabledataOriginSet(data, origin);
    }

    inline ProErrorCode tableDataSizetypeSet(
        RawTableData data, RawTableSizeType type) {
        return ::ProDwgtabledataSizetypeSet(data, type);
    }

    inline ProErrorCode tableDataRowsSet(
        RawTableData data, int rows, double* heights) {
        return ::ProDwgtabledataRowsSet(data, rows, heights);
    }

    inline ProErrorCode tableDataColumnsSet(
        RawTableData data,
        int columns,
        double* widths,
        ProHorzJust* justifications) {
        return ::ProDwgtabledataColumnsSet(
            data, columns, widths, justifications);
    }

    inline ProErrorCode tableCreate(
        RawMdl model,
        RawTableData data,
        RawBoolean display,
        RawTable* p_table) {
        const RawDrawing drawing =
            reinterpret_cast<RawDrawing>(model);

        return ::ProDrawingTableCreate(
            drawing,
            data,
            display,
            p_table);
    }

    inline ProErrorCode tableRowsColumnsCount(
        RawTable* table, int* rows, int* columns) {
        ProErrorCode status = ::ProDwgtableRowsCount(table, rows);
        if (status != kNoError) {
            return status;
        }

        return ::ProDwgtableColumnsCount(table, columns);
    }

    inline ProErrorCode tableColumnWidthSet(
        RawTable* table,
        int column,
        double width,
        RawTableSizeType size_type) {
        return ::ProDwgtableColumnWidthSet(
            table, column, width, size_type);
    }

    inline ProErrorCode tableDisplay(
        RawTable* table) {
        return ::ProDwgtableDisplay(table);
    }

    inline ProErrorCode tableColumnWidthGet(
        RawTable* table,
        int column,
        RawTableSizeType,
        double* width) {
        if (table == nullptr || width == nullptr) {
            return PRO_TK_BAD_INPUTS;
        }

        ProDwgtableInfo info{};
        constexpr int kSingleTableSegment = 0;

        ProErrorCode status = ::ProDwgtableInfoGet(
            table, kSingleTableSegment, &info);
        if (status != kNoError) {
            return status;
        }

        if (info.char_width <= 0.0) {
            return PRO_TK_GENERAL_ERROR;
        }

        double world_width = 0.0;
        status = ::ProDwgtableColumnSizeGet(
            table,
            kSingleTableSegment,
            column - 1,
            &world_width);
        if (status != kNoError) {
            return status;
        }

        *width = world_width / info.char_width;
        return kNoError;
    }

    inline ProErrorCode tableCellsMerge(
        RawTable* table,
        int start_column,
        int start_row,
        int end_column,
        int end_row,
        RawBoolean display) {
        return ::ProDwgtableCellsMerge(
            table,
            start_column,
            start_row,
            end_column,
            end_row,
            display);
    }

    inline ProErrorCode tableCellsRemesh(
        RawTable* table,
        int start_column,
        int start_row,
        int end_column,
        int end_row,
        RawBoolean display) {
        return ::ProDwgtableCellsRemesh(
            table,
            start_column,
            start_row,
            end_column,
            end_row,
            display);
    }

    inline ProErrorCode tableRowHeightAutoAdjustSet(
        RawTable* table,
        int row,
        RawTableRowHeightAutoadjustType value) {
        return ::ProDwgtableRowheightAutoadjustSet(
            table, row, value);
    }

    inline ProErrorCode tableRowAdd(
        RawTable* table,
        int insert_after_row,
        RawBoolean display,
        double height) {
        return ::ProDwgtableRowAdd(
            table, insert_after_row, display, height);
    }

    inline ProErrorCode tableColumnAdd(
        RawTable* table,
        int insert_after_column,
        RawBoolean display,
        double width) {
        return ::ProDwgtableColumnAdd(
            table, insert_after_column, display, width);
    }

    inline ProErrorCode tableRowDelete(
        RawTable* table, int row, RawBoolean display) {
        return ::ProDwgtableRowDelete(table, row, display);
    }

    inline ProErrorCode tableColumnDelete(
        RawTable* table, int column, RawBoolean display) {
        return ::ProDwgtableColumnDelete(table, column, display);
    }

    inline ProErrorCode tableCellTextWrap(
        RawTable* table, int row, int column) {
        return ::ProDwgtableCelltextWrap(table, row, column);
    }

    inline ProErrorCode tableTextEnter(
        RawTable* table,
        int column,
        int row,
        RawWstringProArray text) {
        return ::ProDwgtableTextEnter(
            table, column, row, text);
    }

    inline ProErrorCode tableCellTextGet(
        RawTable* table,
        int column,
        int row,
        RawTableParamMode mode,
        RawWstringProArray* p_lines) {
        return ::ProDwgtableCelltextGet(
            table, column, row, mode, p_lines);
    }

#elif defined(CREO13)

    inline ProErrorCode tableDataAlloc(RawTableData* p_data) {
        return ::ProTableDataAlloc(p_data);
    }

    inline ProErrorCode tableDataOriginSet(
        RawTableData data, RawPoint3d origin) {
        return ::ProTableDataOriginSet(data, origin);
    }

    inline ProErrorCode tableDataSizetypeSet(
        RawTableData data, RawTableSizeType type) {
        return ::ProTableDataSizetypeSet(data, type);
    }

    inline ProErrorCode tableDataRowsSet(
        RawTableData data, int rows, double* heights) {
        return ::ProTableDataRowsSet(data, rows, heights);
    }

    inline ProErrorCode tableDataColumnsSet(
        RawTableData data,
        int columns,
        double* widths,
        ProHorzJust* justifications) {
        return ::ProTableDataColumnsSet(
            data, columns, widths, justifications);
    }

    inline ProErrorCode tableCreate(
        RawMdl model,
        RawTableData data,
        RawBoolean display,
        RawTable* p_table) {
        return ::ProTableCreate(
            model, data, display, p_table);
    }

    inline ProErrorCode tableRowsColumnsCount(
        RawTable* table, int* rows, int* columns) {
        return ::ProTableRowsColumnsCount(
            table, rows, columns);
    }

    inline ProErrorCode tableColumnWidthSet(
        RawTable* table,
        int column,
        double width,
        RawTableSizeType size_type) {
        return ::ProTableColumnWidthSet(
            table, column, width, size_type);
    }

    inline ProErrorCode tableDisplay(
        RawTable* table) {
        return ::ProTableDisplay(table, nullptr);
    }

    inline ProErrorCode tableColumnWidthGet(
        RawTable* table,
        int column,
        RawTableSizeType size_type,
        double* width) {
        return ::ProTableColumnWidthGet(
            table, column, size_type, width);
    }

    inline ProErrorCode tableCellsMerge(
        RawTable* table,
        int start_column,
        int start_row,
        int end_column,
        int end_row,
        RawBoolean display) {
        return ::ProTableCellsMerge(
            table,
            start_column,
            start_row,
            end_column,
            end_row,
            display);
    }

    inline ProErrorCode tableCellMergeGet(
        RawTable* table,
        int row,
        int column,
        RawBoolean* is_merge,
        int* start_row,
        int* start_column,
        int* end_row,
        int* end_column) {
        return ::ProTableCellMergeGet(
            table, row, column, is_merge,
            start_row, start_column, end_row, end_column);
    }

    inline ProErrorCode tableCellsRemesh(
        RawTable* table,
        int start_column,
        int start_row,
        int end_column,
        int end_row,
        RawBoolean display) {
        return ::ProTableCellsRemesh(
            table,
            start_column,
            start_row,
            end_column,
            end_row,
            display);
    }

    inline ProErrorCode tableRowHeightAutoAdjustSet(
        RawTable* table,
        int row,
        RawTableRowHeightAutoadjustType value) {
        return ::ProTableRowheightAutoadjustSet(
            table, row, value);
    }

    inline ProErrorCode tableRowAdd(
        RawTable* table,
        int insert_after_row,
        RawBoolean display,
        double height) {
        return ::ProTableRowAdd(
            table, insert_after_row, display, height);
    }

    inline ProErrorCode tableColumnAdd(
        RawTable* table,
        int insert_after_column,
        RawBoolean display,
        double width) {
        return ::ProTableColumnAdd(
            table, insert_after_column, display, width);
    }

    inline ProErrorCode tableRowDelete(
        RawTable* table, int row, RawBoolean display) {
        return ::ProTableRowDelete(table, row, display);
    }

    inline ProErrorCode tableColumnDelete(
        RawTable* table, int column, RawBoolean display) {
        return ::ProTableColumnDelete(table, column, display);
    }

    inline ProErrorCode tableTextEnter(
        RawTable* table,
        int column,
        int row,
        RawWstringProArray text) {
        return ::ProTableTextEnter(
            table, column, row, text);
    }

    inline ProErrorCode tableCellTextWrap(
        RawTable* table,
        int row,
        int column) {
        return ::ProTableCelltextWrap(
            table, row, column);
    }

    inline ProErrorCode tableCellTextGet(
        RawTable* table,
        int column,
        int row,
        RawTableParamMode mode,
        RawWstringProArray* p_lines) {
        return ::ProTableCelltextGet(
            table, column, row, mode, p_lines);
    }

#endif

    inline ProErrorCode wstringProArrayAlloc(
        int count,
        RawWstringProArray* p_array) {
        if (count < 0 || p_array == nullptr) {
            return PRO_TK_BAD_INPUTS;
        }

        *p_array = nullptr;

        if (count == 0) {
            return kNoError;
        }

        return ::ProArrayAlloc(
            count,
            sizeof(ProWstring),
            1,
            reinterpret_cast<ProArray*>(p_array));
    }

    inline ProErrorCode wstringProArrayFree(
        RawWstringProArray* p_array) {
        if (p_array == nullptr || *p_array == nullptr) {
            return kNoError;
        }

        // The array is owned by the caller and was allocated through
        // wstringProArrayAlloc().
        const ProErrorCode status =
            ::ProArrayFree(
                reinterpret_cast<RawArray*>(p_array));

        if (status == kNoError) {
            *p_array = nullptr;
        }

        return status;
    }

    inline ProErrorCode wstringProArrayOutputFree(
        RawWstringProArray* p_array) {
        if (p_array == nullptr || *p_array == nullptr) {
            return kNoError;
        }

        // Use this only for wide-string arrays returned by Creo TOOLKIT,
        // such as ProTableCelltextGet().
        const ProErrorCode status =
            ::ProWstringproarrayFree(
                reinterpret_cast<wchar_t**>(p_array));

        if (status == kNoError) {
            *p_array = nullptr;
        }

        return status;
    }

    inline ProErrorCode annotationShow(
        RawAnnotation* annotation,
        RawAsmcompPath* comp_path,
        RawView view) {
        return ::ProAnnotationShow(
            annotation,
            comp_path,
            view);
    }

    inline ProErrorCode annotationIsShown(
        RawAnnotation* annotation,
        RawDrawing drawing,
        RawBoolean* is_shown) {
        return ::ProAnnotationIsShown(
            annotation,
            drawing,
            is_shown);
    }

    inline ProErrorCode annotationDisplay(
        RawAnnotation* annotation,
        RawAsmcompPath* comp_path,
        RawDrawing drawing,
        RawView view) {
        return ::ProAnnotationDisplay(
            annotation,
            comp_path,
            drawing,
            view);
    }

    inline ProErrorCode annotationUndisplay(
        RawAnnotation* annotation,
        RawAsmcompPath* comp_path,
        RawDrawing drawing) {
        return ::ProAnnotationUndisplay(
            annotation,
            comp_path,
            drawing);
    }

    inline ProErrorCode annotationElementGet(
        RawAnnotation* annotation,
        RawAnnotationElem* element) {
        return ::ProAnnotationElementGet(
            annotation,
            element);
    }

    inline ProErrorCode annotationIsInactive(
        RawAnnotation* annotation,
        RawBoolean* is_inactive) {
        return ::ProAnnotationIsInactive(
            annotation,
            is_inactive);
    }

    inline ProErrorCode annotationRotate(
        RawAnnotation* annotation,
        double rotation) {
        return ::ProAnnotationRotate(
            annotation,
            rotation);
    }

    inline ProErrorCode annotationIsAssociative(
        RawAnnotation* annotation,
        RawDrawing drawing,
        RawBoolean* assoc_position,
        RawAnnotationAttachmentAssociativity* assoc_attach) {
        return ::ProAnnotationIsAssociative(
            annotation,
            drawing,
            assoc_position,
            assoc_attach,
            nullptr);
    }

    inline ProErrorCode annotationPositionUpdate(
        RawAnnotation* annotation,
        RawDrawing drawing) {
        return ::ProAnnotationPositionUpdate(
            annotation,
            drawing);
    }

    inline ProErrorCode annotationAttachmentUpdate(
        RawAnnotation* annotation,
        RawDrawing drawing) {
        return ::ProAnnotationAttachmentUpdate(
            annotation,
            drawing);
    }

    inline ProErrorCode annotationUpdate(
        RawAnnotation* annotation,
        RawDrawing drawing) {
        return ::ProAnnotationUpdate(
            annotation,
            drawing);
    }

    inline ProErrorCode drawingAnnotationErase(
        RawDrawing drawing,
        RawAnnotation* annotation) {
        return ::ProDrawingAnnotationErase(
            drawing,
            annotation);
    }

    inline ProErrorCode annotationByViewShow(
        RawDrawing drawing,
        RawView view,
        RawObjectType annotation_type) {
        return ::ProAnnotationByViewShow(
            drawing,
            view,
            annotation_type);
    }

    inline ProErrorCode annotationByFeatureShow(
        RawDrawing drawing,
        RawSelection feature_selection,
        RawView view,
        RawObjectType annotation_type) {
        return ::ProAnnotationByFeatureShow(
            drawing,
            feature_selection,
            view,
            annotation_type);
    }

    inline ProErrorCode annotationByComponentShow(
        RawDrawing drawing,
        RawSelection component_selection,
        RawView view,
        RawObjectType annotation_type) {
        return ::ProAnnotationByComponentShow(
            drawing,
            component_selection,
            view,
            annotation_type);
    }

    inline ProErrorCode annotationNeedsConversion(
        RawAnnotation* annotation,
        RawBoolean* needs_conversion) {
        return ::ProAnnotationNeedsConversion(
            annotation,
            needs_conversion);
    }

    inline ProErrorCode annotationLegacyConvert(
        RawAnnotation* annotation) {
        return ::ProAnnotationLegacyConvert(
            annotation);
    }


    inline ProErrorCode engineerConnectIdGet(
        char* connect_id) {
        return ::ProEngineerConnectIdGet(connect_id);
    }

}
