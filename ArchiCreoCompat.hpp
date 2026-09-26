#pragma once

#include <ProArray.h>
#include <ProAssembly.h>
#include <ProCore.h>
#include <ProMdl.h>
#include <ProModelitem.h>
#include <ProSolid.h>
#include <ProToolkit.h>
#include <ProUtil.h>
#include <ProWindows.h>

namespace creo::detail {

    using ProErrorCode = ::ProError;
    using RawMdl = ::ProMdl;
    using RawArray = ::ProArray;
    using RawBoolean = ::ProBoolean;
    using RawModelItem = ::ProModelitem;
    using RawMdlType = ::ProMdlType;
    using RawMdlName = ::ProMdlName;
    using RawMdlFileType = ::ProMdlfileType;
    using RawMdlFileRetrieveOpt = ::ProFileRetrieveOpt;
    using RawObjectType = ::ProType;
    using RawSolid = ::ProSolid;

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

    inline ProErrorCode mdlWindowGet(
        RawMdl model,
        int* window_id) {
        return ::ProMdlWindowGet(model, window_id);
    }

    inline ProErrorCode mdlSave(RawMdl model) {
        return ::ProMdlSave(model);
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

    inline ProErrorCode engineerConnectIdGet(
        char* connect_id) {
        return ::ProEngineerConnectIdGet(connect_id);
    }

}
