#include "ArchiCreoErrorHandler.hpp"

#include <string>
#include <string_view>

namespace creo {

    namespace {

        std::string makeMessage(ErrorCode code, std::string_view context) {
            std::string message = "ProTOOLKIT error: " + toString(code);
            if (!context.empty()) {
                message += " (";
                message += context;
                message += ")";
            }
            return message;
        }

    }

    ProToolkitError::ProToolkitError(ErrorCode code, std::string_view context) :
        std::runtime_error(makeMessage(code, context)), code_(code) {
    }

    std::string toString(ErrorCode code) {
        switch (static_cast<int>(code)) {
            case 0:
                return "PRO_TK_NO_ERROR";
            case -1:
                return "PRO_TK_GENERAL_ERROR";
            case -2:
                return "PRO_TK_BAD_INPUTS";
            case -3:
                return "PRO_TK_USER_ABORT";
            case -4:
                return "PRO_TK_E_NOT_FOUND";
            case -5:
                return "PRO_TK_E_FOUND";
            case -6:
                return "PRO_TK_LINE_TOO_LONG";
            case -7:
                return "PRO_TK_CONTINUE";
            case -8:
                return "PRO_TK_BAD_CONTEXT";
            case -9:
                return "PRO_TK_NOT_IMPLEMENTED";
            case -10:
                return "PRO_TK_OUT_OF_MEMORY";
            case -11:
                return "PRO_TK_COMM_ERROR";
            case -12:
                return "PRO_TK_NO_CHANGE";
            case -13:
                return "PRO_TK_SUPP_PARENTS";
            case -14:
                return "PRO_TK_PICK_ABOVE";
            case -15:
                return "PRO_TK_INVALID_DIR";
            case -16:
                return "PRO_TK_INVALID_FILE";
            case -17:
                return "PRO_TK_CANT_WRITE";
            case -18:
                return "PRO_TK_INVALID_TYPE";
            case -19:
                return "PRO_TK_INVALID_PTR";
            case -20:
                return "PRO_TK_UNAV_SEC";
            case -21:
                return "PRO_TK_INVALID_MATRIX";
            case -22:
                return "PRO_TK_INVALID_NAME";
            case -23:
                return "PRO_TK_NOT_EXIST";
            case -24:
                return "PRO_TK_CANT_OPEN";
            case -25:
                return "PRO_TK_ABORT";
            case -26:
                return "PRO_TK_NOT_VALID";
            case -27:
                return "PRO_TK_INVALID_ITEM";
            case -28:
                return "PRO_TK_MSG_NOT_FOUND";
            case -29:
                return "PRO_TK_MSG_NO_TRANS";
            case -30:
                return "PRO_TK_MSG_FMT_ERROR";
            case -31:
                return "PRO_TK_MSG_USER_QUIT";
            case -32:
                return "PRO_TK_MSG_TOO_LONG";
            case -33:
                return "PRO_TK_CANT_ACCESS";
            case -34:
                return "PRO_TK_OBSOLETE_FUNC";
            case -35:
                return "PRO_TK_NO_COORD_SYSTEM";
            case -36:
                return "PRO_TK_E_AMBIGUOUS";
            case -37:
                return "PRO_TK_E_DEADLOCK";
            case -38:
                return "PRO_TK_E_BUSY";
            case -39:
                return "PRO_TK_E_IN_USE";
            case -40:
                return "PRO_TK_NO_LICENSE";
            case -41:
                return "PRO_TK_BSPL_UNSUITABLE_DEGREE";
            case -42:
                return "PRO_TK_BSPL_NON_STD_END_KNOTS";
            case -43:
                return "PRO_TK_BSPL_MULTI_INNER_KNOTS";
            case -44:
                return "PRO_TK_BSPL_BAD_SRF_CRV";
            case -45:
                return "PRO_TK_EMPTY";
            case -46:
                return "PRO_TK_BAD_DIM_ATTACH";
            case -47:
                return "PRO_TK_NOT_DISPLAYED";
            case -48:
                return "PRO_TK_CANT_MODIFY";
            case -49:
                return "PRO_TK_CHECKOUT_CONFLICT";
            case -50:
                return "PRO_TK_CRE_VIEW_BAD_SHEET";
            case -51:
                return "PRO_TK_CRE_VIEW_BAD_MODEL";
            case -52:
                return "PRO_TK_CRE_VIEW_BAD_PARENT";
            case -53:
                return "PRO_TK_CRE_VIEW_BAD_TYPE";
            case -54:
                return "PRO_TK_CRE_VIEW_BAD_EXPLODE";
            case -55:
                return "PRO_TK_UNATTACHED_FEATS";
            case -56:
                return "PRO_TK_REGEN_AGAIN";
            case -57:
                return "PRO_TK_DWGCREATE_ERRORS";
            case -58:
                return "PRO_TK_UNSUPPORTED";
            case -59:
                return "PRO_TK_NO_PERMISSION";
            case -60:
                return "PRO_TK_AUTHENTICATION_FAILURE";
            case -61:
                return "PRO_TK_OUTDATED";
            case -62:
                return "PRO_TK_INCOMPLETE";
            case -63:
                return "PRO_TK_CHECK_OMITTED";
            case -64:
                return "PRO_TK_MAX_LIMIT_REACHED";
            case -65:
                return "PRO_TK_OUT_OF_RANGE";
            case -66:
                return "PRO_TK_CHECK_LAST_ERROR";
            case -67:
                return "PRO_TK_NO_PLM_LICENSE";
            case -68:
                return "PRO_TK_INCOMPLETE_TESS";
            case -69:
                return "PRO_TK_MULTIBODY_UNSUPPORTED";
            case -70:
                return "PRO_TK_BROWSER_UNAVAILABLE";
            case -71:
                return "PRO_TK_DLL_LOAD_ERROR";
            case -88:
                return "PRO_TK_APP_CREO_BARRED";
            case -89:
                return "PRO_TK_APP_TOO_OLD";
            case -90:
                return "PRO_TK_APP_BAD_DATAPATH";
            case -91:
                return "PRO_TK_APP_BAD_ENCODING";
            case -92:
                return "PRO_TK_APP_NO_LICENSE";
            case -93:
                return "PRO_TK_APP_XS_CALLBACKS";
            case -94:
                return "PRO_TK_APP_STARTUP_FAIL";
            case -95:
                return "PRO_TK_APP_INIT_FAIL";
            case -96:
                return "PRO_TK_APP_VERSION_MISMATCH";
            case -97:
                return "PRO_TK_APP_COMM_FAILURE";
            case -98:
                return "PRO_TK_APP_NEW_VERSION";
            case -99:
                return "PRO_TK_APP_UNLOCK";
            case -100:
                return "PRO_TK_APP_JLINK_NOT_ALLOWED";
            default:
                return "code #" + std::to_string(static_cast<int>(code)) + " (unknown to creo::toString)";
        }
    }

}
