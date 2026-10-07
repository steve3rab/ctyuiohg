#pragma once

#include <ProArray.h>
#include <ProAsmcomp.h>
#include <ProAsmcomppath.h>
#include <ProAssembly.h>
#include <ProCore.h>
#include <ProMdl.h>
#include <ProModelitem.h>
#include <ProParameter.h>
#include <ProSolid.h>
#include <ProTable.h>
#include <ProToolkit.h>
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
    using RawObjectType = ::ProType;
    using RawSolid = ::ProSolid;
    using RawAssembly = ::ProAssembly;
    using RawAsmcomp = ::ProAsmcomp;
    using RawMatrix = ::ProMatrix;
    using RawVector = ::ProVector;
    using RawPoint3d = ::ProPoint3d;
    using RawTable = ::ProTable;
    using RawTableData = ::ProTableData;
    using RawWstringProArray = ::ProWstring*;

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
        return ::ProArrayFree(p_array);
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
        const void* value,
        RawParamValueType type) {
        return ::ProParamvalueSet(p_handle, value, type);
    }

    inline ProErrorCode paramvalueValueGet(
        const RawParamValue* p_handle,
        RawParamValueType type,
        void* p_value) {
        return ::ProParamvalueValueGet(p_handle, type, p_value);
    }

    inline ProErrorCode paramvalueTypeGet(
        const RawParamValue* p_handle,
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

        return ::ProArrayFree(
            reinterpret_cast<RawArray*>(
                p_model_array));
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

    inline RawSolid mdlToSolid(RawMdl model) {
        return static_cast<RawSolid>(model);
    }

    inline ProErrorCode stringFree(char* string) {
        return ::ProStringFree(string);
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


    inline ProErrorCode tableDataAlloc(RawTableData* p_data) {
        return ::ProTableDataAlloc(p_data);
    }

    inline ProErrorCode tableDataFree(RawTableData* p_data) {
        return ::ProTableDataFree(p_data);
    }

    inline ProErrorCode tableDataOriginSet(RawTableData data, RawPoint3d origin) {
        return ::ProTableDataOriginSet(data, origin);
    }

    inline ProErrorCode tableDataSizetypeSet(RawTableData data, ProTableSizetype type) {
        return ::ProTableDataSizetypeSet(data, type);
    }

    inline ProErrorCode tableDataRowsSet(RawTableData data, int rows, double* heights) {
        return ::ProTableDataRowsSet(data, rows, heights);
    }

    inline ProErrorCode tableDataColumnsSet(
        RawTableData data, int columns, double* widths, ProHorzJust* justifications) {
        return ::ProTableDataColumnsSet(data, columns, widths, justifications);
    }

    inline ProErrorCode tableCreate(
        RawMdl model, RawTableData data, RawBoolean display, RawTable* p_table) {
        return ::ProTableCreate(model, data, display, p_table);
    }

    inline ProErrorCode tableRowsColumnsCount(
        RawTable* table, int* rows, int* columns) {
        return ::ProTableRowsColumnsCount(table, rows, columns);
    }

    inline ProErrorCode tableColumnWidthSet(
        RawTable* table, int column, double width, ProBoolean characters) {
        return ::ProTableColumnWidthSet(table, column, width, characters);
    }

    inline ProErrorCode tableColumnWidthGet(
        RawTable* table, int column, ProBoolean characters, double* width) {
        return ::ProTableColumnWidthGet(table, column, characters, width);
    }

    inline ProErrorCode tableCellTextWrap(
        RawTable* table, int row, int column) {
        return ::ProTableCelltextWrap(table, row, column);
    }

    inline ProErrorCode tableRowHeightAutoAdjustSet(
        RawTable* table, int row, ProTblRowheightAutoAdjust value) {
        return ::ProTableRowheightAutoadjustSet(table, row, value);
    }

    inline ProErrorCode tableRowAdd(
        RawTable* table, int insert_after_row, RawBoolean display, double height) {
        return ::ProTableRowAdd(table, insert_after_row, display, height);
    }

    inline ProErrorCode tableColumnAdd(
        RawTable* table, int insert_after_column, RawBoolean display, double width) {
        return ::ProTableColumnAdd(table, insert_after_column, display, width);
    }

    inline ProErrorCode tableRowDelete(
        RawTable* table, int row, RawBoolean display) {
        return ::ProTableRowDelete(table, row, display);
    }

    inline ProErrorCode tableColumnDelete(
        RawTable* table, int column, RawBoolean display) {
        return ::ProTableColumnDelete(table, column, display);
    }

    inline ProErrorCode wstringProArrayAlloc(RawWstringProArray* p_array) {
        return ::ProArrayAlloc(1, sizeof(ProWstring), 1,
            reinterpret_cast<ProArray*>(p_array));
    }

    inline ProErrorCode wstringProArrayFree(RawWstringProArray* p_array) {
        return ::ProArrayFree(reinterpret_cast<ProArray*>(p_array));
    }

    inline ProErrorCode tableTextEnter(
        RawTable* table, int column, int row, RawWstringProArray text) {
        return ::ProTableTextEnter(table, column, row, text);
    }

    inline ProErrorCode engineerConnectIdGet(
        char* connect_id) {
        return ::ProEngineerConnectIdGet(connect_id);
    }

}
