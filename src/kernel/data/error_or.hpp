#ifndef DATA_ERROR_OR_INCLUDED
#define DATA_ERROR_OR_INCLUDED

#include "error.hpp"

namespace Kernel::Data {

    // Thanks SerenityOS for the idea!

    template<typename Ty, typename ErrorTy=Error>
    class ErrorOr {
    public:
        ErrorOr(Ty t_value) : m_isError(false) {
            m_contained.val = t_value;
        }

        ErrorOr(ErrorTy t_error) : m_isError(true) {
            m_contained.err = t_error;
        }

        [[nodiscard]] bool is_error() const {
            return m_isError;
        }

        [[nodiscard]] Ty get_value() const {
            return m_contained.val;
        }

        [[nodiscard]] ErrorTy get_error() const {
            return m_contained.err;
        }

    private:
        union {
            Ty val;
            ErrorTy err;
        } m_contained;
        bool m_isError;
    };

    template<typename ErrorTy>
    class ErrorOr<void, ErrorTy> {
    public:
        ErrorOr() : m_isError(false) {
            ;
        }

        ErrorOr(ErrorTy t_error) : m_error(t_error), m_isError(true) {
            ;
        }

        [[nodiscard]] bool is_error() const {
            return m_isError;
        }

        void get_value() const {
            ;
        }

        [[nodiscard]] ErrorTy get_error() const {
            return m_error;
        }

    private:
        ErrorTy m_error;
        bool m_isError;
    };

}

// This uses a GCC extension!
#define TRY(x) ({\
    const auto _errorOrX = (x);\
    if (_errorOrX.is_error()) {\
        return _errorOrX.get_error();\
    }\
    _errorOrX.get_value();\
})

#define ASSERT(x, e) do {\
    if (!(x)) {\
        return (e);\
    }\
} while(false)


#endif
