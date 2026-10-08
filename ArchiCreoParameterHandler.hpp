#pragma once

#include <cwchar>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "ArchiCreoCompat.hpp"
#include "ArchiCreoErrorHandler.hpp"
#include "ArchiCreoModelHandler.hpp"

namespace creo {

    struct ParameterNoteId final {
        int value = 0;

        friend bool operator==(
            const ParameterNoteId& lhs,
            const ParameterNoteId& rhs) noexcept {
            return lhs.value == rhs.value;
        }
    };

    enum class ParameterType {
        Double,
        String,
        Integer,
        Boolean,
        NoteId,
        Void
    };

    using ParameterValue = std::variant<
        double,
        std::wstring,
        int,
        bool,
        ParameterNoteId>;

    struct ParameterInfo final {
        std::wstring name;
        ParameterType type = ParameterType::Void;
        ParameterValue value = std::wstring{};
        detail::RawLockStatus lockStatus =
            PRO_PARAMLOCKSTATUS_UNLOCKED;
    };

    class ArchiCreoParameterHandler final {
      public:
        explicit ArchiCreoParameterHandler(
            detail::RawMdl model) noexcept :
            model_(model) {
        }

        explicit ArchiCreoParameterHandler(
            const ArchiCreoModelHandler& model) noexcept :
            model_(model.raw()) {
        }

        [[nodiscard]] detail::RawMdl rawModel() const noexcept {
            return model_;
        }

        [[nodiscard]] bool isValid() const noexcept {
            return model_ != nullptr;
        }

        [[nodiscard]] bool exists(
            const std::wstring& name) const {
            return initializeParameter(name, nullptr);
        }

        [[nodiscard]] bool isModifiable(
            const std::wstring& name) const {
            detail::RawParameter parameter{};
            if (!initializeParameter(name, &parameter)) {
                return false;
            }

            detail::RawLockStatus status =
                PRO_PARAMLOCKSTATUS_LOCKED;

            CREO_CHECK(
                detail::parameterLockstatusGet(
                    &parameter,
                    &status));

            return status != PRO_PARAMLOCKSTATUS_LOCKED;
        }

        [[nodiscard]] ParameterInfo get(
            const std::wstring& name) const {
            detail::RawParameter parameter =
                initializeExistingParameter(name);

            detail::RawParamValue value{};
            CREO_CHECK(
                detail::parameterValueGet(
                    &parameter,
                    &value));

            ParameterInfo info{};
            info.name = name;
            info.type = parameterType(value.type);
            info.value = toParameterValue(value);
            info.lockStatus = lockStatus(parameter);
            return info;
        }

        void add(
            const std::wstring& name,
            const ParameterValue& value) const {
            requireModel();

            if (name.empty()) {
                throw std::invalid_argument(
                    "ArchiCreoParameterHandler::add: "
                    "parameter name cannot be empty");
            }

            if (exists(name)) {
                throw std::invalid_argument(
                    "ArchiCreoParameterHandler::add: "
                    "parameter already exists");
            }

            detail::RawModelItem owner{};
            CREO_CHECK(
                detail::mdlToModelitem(
                    model_, &owner));

            detail::RawName parameterName{};
            copyName(name, parameterName);

            detail::RawParamValue parameterValue =
                makeParameterValue(value);

            detail::RawParameter parameter{};
            CREO_CHECK(
                detail::parameterCreate(
                    &owner,
                    parameterName,
                    &parameterValue,
                    &parameter));
        }

        void set(
            const std::wstring& name,
            const ParameterValue& value) const {
            detail::RawParameter parameter =
                initializeExistingParameter(name);

            detail::RawParamValue current{};
            CREO_CHECK(
                detail::parameterValueGet(
                    &parameter,
                    &current));

            const ParameterType currentType =
                parameterType(current.type);
            const ParameterType newType =
                parameterType(
                    makeParameterValue(value).type);

            if (currentType != newType) {
                throw std::invalid_argument(
                    "ArchiCreoParameterHandler::set: "
                    "parameter type cannot be changed");
            }

            detail::RawLockStatus status =
                PRO_PARAMLOCKSTATUS_LOCKED;

            CREO_CHECK(
                detail::parameterLockstatusGet(
                    &parameter,
                    &status));

            if (status == PRO_PARAMLOCKSTATUS_LOCKED) {
                throw std::runtime_error(
                    "ArchiCreoParameterHandler::set: "
                    "parameter is locked");
            }

            detail::RawParamValue newValue =
                makeParameterValue(value);

            CREO_CHECK(
                detail::parameterValueSet(
                    &parameter,
                    &newValue));
        }

        void remove(
            const std::wstring& name) const {
            detail::RawParameter parameter =
                initializeExistingParameter(name);

            detail::RawLockStatus status =
                PRO_PARAMLOCKSTATUS_LOCKED;

            CREO_CHECK(
                detail::parameterLockstatusGet(
                    &parameter,
                    &status));

            if (status == PRO_PARAMLOCKSTATUS_LOCKED) {
                throw std::runtime_error(
                    "ArchiCreoParameterHandler::remove: "
                    "parameter is locked");
            }

            CREO_CHECK(
                detail::parameterDelete(
                    &parameter));
        }

      private:
        detail::RawMdl model_;

        void requireModel() const {
            if (!isValid()) {
                throw std::runtime_error(
                    "ArchiCreoParameterHandler: "
                    "invalid model handle");
            }
        }

        static void copyName(
            const std::wstring& source,
            detail::RawName destination) {
            if (source.size() >= PRO_NAME_SIZE) {
                throw std::invalid_argument(
                    "ArchiCreoParameterHandler: "
                    "parameter name is too long");
            }

            std::wcsncpy(
                destination,
                source.c_str(),
                PRO_NAME_SIZE - 1);
            destination[PRO_NAME_SIZE - 1] = L'\0';
        }

        bool initializeParameter(
            const std::wstring& name,
            detail::RawParameter* output) const {
            requireModel();

            if (name.empty()) {
                throw std::invalid_argument(
                    "ArchiCreoParameterHandler: "
                    "parameter name cannot be empty");
            }

            detail::RawModelItem owner{};
            CREO_CHECK(
                detail::mdlToModelitem(
                    model_, &owner));

            detail::RawName parameterName{};
            copyName(name, parameterName);

            detail::RawParameter parameter{};
            const detail::ProErrorCode status =
                detail::parameterInit(
                    &owner,
                    parameterName,
                    &parameter);

            if (status == PRO_TK_E_NOT_FOUND) {
                return false;
            }

            CREO_CHECK(status);

            if (output != nullptr) {
                *output = parameter;
            }

            return true;
        }

        detail::RawParameter initializeExistingParameter(
            const std::wstring& name) const {
            detail::RawParameter parameter{};

            if (!initializeParameter(name, &parameter)) {
                throw std::out_of_range(
                    "ArchiCreoParameterHandler: "
                    "parameter does not exist");
            }

            return parameter;
        }

        static detail::RawLockStatus lockStatus(
            detail::RawParameter& parameter) {
            detail::RawLockStatus status =
                PRO_PARAMLOCKSTATUS_LOCKED;

            CREO_CHECK(
                detail::parameterLockstatusGet(
                    &parameter,
                    &status));

            return status;
        }

        static ParameterType parameterType(
            detail::RawParamValueType type) {
            switch (type) {
                case PRO_PARAM_DOUBLE:
                    return ParameterType::Double;
                case PRO_PARAM_STRING:
                    return ParameterType::String;
                case PRO_PARAM_INTEGER:
                    return ParameterType::Integer;
                case PRO_PARAM_BOOLEAN:
                    return ParameterType::Boolean;
                case PRO_PARAM_NOTE_ID:
                    return ParameterType::NoteId;
                case PRO_PARAM_VOID:
                case PRO_PARAM_NOT_SET:
                    return ParameterType::Void;
                default:
                    throw std::runtime_error(
                        "ArchiCreoParameterHandler: "
                        "unsupported Creo parameter type");
            }
        }

        static detail::RawParamValue makeParameterValue(
            const ParameterValue& value) {
            detail::RawParamValue result{};

            std::visit(
                [&result](const auto& typedValue) {
                    using T = std::decay_t<decltype(typedValue)>;

                    if constexpr (std::is_same_v<T, double>) {
                        double mutableValue = typedValue;

                        CREO_CHECK(
                            detail::paramvalueSet(
                                &result,
                                &mutableValue,
                                PRO_PARAM_DOUBLE));
                    } else if constexpr (
                        std::is_same_v<T, std::wstring>) {
                        std::wstring mutableValue = typedValue;

                        CREO_CHECK(
                            detail::paramvalueSet(
                                &result,
                                mutableValue.data(),
                                PRO_PARAM_STRING));
                    } else if constexpr (
                        std::is_same_v<T, int>) {
                        int mutableValue = typedValue;

                        CREO_CHECK(
                            detail::paramvalueSet(
                                &result,
                                &mutableValue,
                                PRO_PARAM_INTEGER));
                    } else if constexpr (
                        std::is_same_v<T, bool>) {
                        short mutableValue =
                            typedValue ? 1 : 0;

                        CREO_CHECK(
                            detail::paramvalueSet(
                                &result,
                                &mutableValue,
                                PRO_PARAM_BOOLEAN));
                    } else if constexpr (
                        std::is_same_v<T, ParameterNoteId>) {
                        int mutableValue = typedValue.value;

                        CREO_CHECK(
                            detail::paramvalueSet(
                                &result,
                                &mutableValue,
                                PRO_PARAM_NOTE_ID));
                    }
                },
                value);

            return result;
        }

        static ParameterValue toParameterValue(
            detail::RawParamValue& value) {
            switch (value.type) {
                case PRO_PARAM_DOUBLE: {
                    double result = 0.0;
                    CREO_CHECK(
                        detail::paramvalueValueGet(
                            &value,
                            value.type,
                            &result));
                    return result;
                }

                case PRO_PARAM_STRING: {
                    wchar_t buffer[PRO_LINE_SIZE] = {};
                    CREO_CHECK(
                        detail::paramvalueValueGet(
                            &value,
                            value.type,
                            buffer));
                    return std::wstring(buffer);
                }

                case PRO_PARAM_INTEGER: {
                    int result = 0;
                    CREO_CHECK(
                        detail::paramvalueValueGet(
                            &value,
                            value.type,
                            &result));
                    return result;
                }

                case PRO_PARAM_BOOLEAN: {
                    short result = 0;
                    CREO_CHECK(
                        detail::paramvalueValueGet(
                            &value,
                            value.type,
                            &result));
                    return result != 0;
                }

                case PRO_PARAM_NOTE_ID: {
                    int result = 0;
                    CREO_CHECK(
                        detail::paramvalueValueGet(
                            &value,
                            value.type,
                            &result));
                    return ParameterNoteId{result};
                }

                case PRO_PARAM_VOID:
                case PRO_PARAM_NOT_SET:
                    throw std::runtime_error(
                        "ArchiCreoParameterHandler: "
                        "parameter has no supported value");

                default:
                    throw std::runtime_error(
                        "ArchiCreoParameterHandler: "
                        "unsupported Creo parameter type");
            }
        }
    };

}
