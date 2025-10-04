
#pragma once

#include <cstddef>
#include <etl/type_traits.h>

using StatusStr_t = const char*;

#define ETFW_STATUS_INVALID_CODE_STR    "Status unknown. Invalid code"

template <typename CodeTrait>
class EtfwStatus final
{
    public:
        using Code = typename CodeTrait::Code;

        // Type checks
        static_assert(sizeof(Code) <= sizeof(int),
            "Code enum must be trivially copyable");
        static_assert(etl::underlying_type_t<Code>(Code::OK) == 0,
            "Status code must have 'OK' value as '0'");
        static_assert(etl::underlying_type_t<Code>(Code::COUNT) > 0,
            "Status code must have 'COUNT' value greater than 0");
        static_assert((sizeof(CodeTrait::ErrStrLkup)/sizeof(CodeTrait::ErrStrLkup[0])) == 
                (etl::underlying_type_t<Code>(Code::COUNT)),
            "Status string table must have 'COUNT' number of entries");

        /// @brief Construct status from code
        /// @param code Status code
        EtfwStatus(Code code):
            code_(code)
        {}

        /// @brief Default status constructor. Defaults to 'OK'
        EtfwStatus():
            code_(Code::OK)
        {}

        /// @brief Checks if operation succeeded.
        /// @return true Operation was successful.
        /// @return false Operation was unsuccessful.
        inline bool success(void) const
        {
            return code_ == Code::OK;
        }

        ///@brief Checks if operation failed.
        ///@return true Operation was unsuccessful.
        ///@return false Operation was successful.
        inline bool error(void) const
        {
            return code_ != Code::OK;
        }

        /// @brief Returns the status description.
        /// @return const char* Raw string describing the status.
        inline StatusStr_t str(void) const
        {
            if (code_ < Code::COUNT)
            {
                return CodeTrait::ErrStrLkup[static_cast<size_t>(code_)];
            }
            else
            {
                return unknown_code_str_;
            }
        }

        /// @brief Gets the code value.
        /// @return Code Value.
        inline Code code(void) const { return code_; }

        operator bool() const { return code_ == Code::OK; }

    private:
        Code code_;

        static constexpr StatusStr_t unknown_code_str_ = ETFW_STATUS_INVALID_CODE_STR;
};
