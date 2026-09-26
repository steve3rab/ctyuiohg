#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include "ArchiCreoCompat.hpp"

namespace creo {

    using ErrorCode = detail::ProErrorCode;

    class ProToolkitError : public std::runtime_error {
      public:
        ProToolkitError(ErrorCode code, std::string_view context);

        ErrorCode code() const noexcept {
            return code_;
        }

      private:
        ErrorCode code_;
    };

    std::string toString(ErrorCode code);

    inline void throwIfError(ErrorCode code, std::string_view context = {}) {
        if (code != detail::kNoError) {
            throw ProToolkitError(code, context);
        }
    }

}

#define CREO_CHECK(expr) ::creo::throwIfError((expr), #expr)
