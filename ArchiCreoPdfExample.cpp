/*
 * Example: rewrite of the legacy PDF export block using the
 * ArchiCreoPdfHandler / ArchiCreoPdfOptions wrappers.
 *
 * The PDF options and export behavior are intentionally unchanged.
 *
 * Expected legacy inputs:
 *   p_Mdl      : current ProMdl to export
 *   mdltype    : ProMdlType of p_Mdl
 *   w_name     : destination PDF path
 *   size_x/y   : 3D paper size
 *   pdf_title  : UTF-8/ACP string used by the existing application
 *   pdf_author : UTF-8/ACP string used by the existing application
 */

#include "ArchiCreoPdfHandler.hpp"
#include "ArchiCreoModelHandler.hpp"
#include "ArchiPropertyUtils.hpp"

#include <filesystem>
#include <string>

ProError exportPdfExample(
    ProMdl p_Mdl,
    ProMdlType mdltype,
    const wchar_t* w_name,
    double size_x,
    double size_y,
    const char* pdf_title,
    const char* pdf_author)
{
    if (p_Mdl == nullptr ||
        w_name == nullptr) {
        return PRO_TK_BAD_INPUTS;
    }

    try {
        creo::ArchiCreoModelHandler model(p_Mdl);

        creo::ArchiCreoPdfOptions pdfOptions;

        /* Specify 2D or 3D PDF according to the model type */
        if (mdltype == PRO_MDL_DRAWING) {
            pdfOptions.setInt(
                PRO_PDFOPT_EXPORT_MODE,
                PRO_PDF_2D_DRAWING);
        } else {
            pdfOptions.setInt(
                PRO_PDFOPT_EXPORT_MODE,
                PRO_PDF_3D_AS_U3D_PDF);
        }

        /* Force paper size for 3D */
        if (mdltype != PRO_MDL_DRAWING) {
            pdfOptions.setInt(
                PRO_PDFOPT_SIZE,
                VARIABLE_SIZE_PLOT);

            pdfOptions.setDouble(
                PRO_PDFOPT_WIDTH,
                size_x);

            pdfOptions.setDouble(
                PRO_PDFOPT_HEIGHT,
                size_y);
        }

        /* Stroke the fonts */
        pdfOptions.setInt(
            PRO_PDFOPT_FONT_STROKE,
            PRO_PDF_STROKE_ALL_FONTS);

        pdfOptions.setBool(
            PRO_PDFOPT_SEARCHABLE_TEXT,
            true);

        /* Treat the end lines */
        pdfOptions.setInt(
            PRO_PDFOPT_LINECAP,
            PRO_PDF_LINECAP_ROUND);

        pdfOptions.setInt(
            PRO_PDFOPT_LINEJOIN,
            PRO_PDF_LINEJOIN_ROUND);

        /* Use the Pro/E pentable */
        pdfOptions.setBool(
            PRO_PDFOPT_PENTABLE,
            true);

        /* Use the Pro/E parameter mode */
        pdfOptions.setInt(
            PRO_PDFOPT_PARAM_MODE,
            PRO_PDF_PARAMS_ALL);

        /* Add the Title */
        if (pdf_title != nullptr && *pdf_title != '\0') {
            pdfOptions.setString(
                PRO_PDFOPT_TITLE,
                ArchiPropertyUtils::charToWideString(
                    pdf_title));
        }

        /* Add the Author */
        if (pdf_author != nullptr && *pdf_author != '\0') {
            pdfOptions.setString(
                PRO_PDFOPT_AUTHOR,
                ArchiPropertyUtils::charToWideString(
                    pdf_author));
        }

        /* Do not launch Acrobat Reader */
        pdfOptions.setBool(
            PRO_PDFOPT_LAUNCH_VIEWER,
            false);

        /*
         * Keep the original mdltype-driven behavior.
         * The wrapper converts the native path to ProPath and calls ProPDFExport.
         */
        creo::ArchiCreoPdfHandler::exportModel(
            model,
            std::filesystem::path(w_name),
            pdfOptions);

        return PRO_TK_NO_ERROR;
    }
    catch (const creo::ProToolkitError& error) {
        return error.code();
    }
    catch (...) {
        return PRO_TK_GENERAL_ERROR;
    }
}
