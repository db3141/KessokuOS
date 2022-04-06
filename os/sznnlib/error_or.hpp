#ifndef SZNN_ERROR_OR_INCLUDED
#define SZNN_ERROR_OR_INCLUDED

namespace SZNN {

    // Thanks SerenityOS for the idea!

    template<typename Ty, typename Error=int>
    class ErrorOr {
    public:
        ErrorOr(Ty t_value) : m_isError(false) {
            m_contained.val = t_value;
        }

        ErrorOr(Error t_error) : m_isError(true) {
            m_contained.err = t_error;
        }

        [[nodiscard]] bool is_error() const {
            return m_isError;
        }

        [[nodiscard]] Ty get_value() const {
            return m_contained.val;
        }

        [[nodiscard]] Error get_error() const {
            return m_contained.err;
        }

    private:
        union {
            Ty val;
            Error err;
        } m_contained;
        bool m_isError;
    };

    template<typename Error>
    class ErrorOr<void, Error> {
    public:
        ErrorOr() : m_isError(false) {
            ;
        }

        ErrorOr(Error t_error) : m_error(t_error), m_isError(true) {
            ;
        }

        [[nodiscard]] bool is_error() const {
            return m_isError;
        }

        void get_value() const {
            ;
        }

        [[nodiscard]] Error get_error() const {
            return m_error;
        }

    private:
        Error m_error;
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
