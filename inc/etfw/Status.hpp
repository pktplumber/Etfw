
#pragma once

using StatusStr_t = const char*;

template <typename CodeTrait>
class EtfwStatus final
{
    public:
        using Code = typename CodeTrait::Code;
        static_assert(sizeof(Code) <= sizeof(int),
            "Code enum must be trivially copyable");

        EtfwStatus(Code code):
            code_(code) {}

        EtfwStatus():
            code_(Code::OK) {}

        /**
         * @brief Checks if operation succeeded.
         * 
         * @return true Operation was successful.
         * @return false Operation was unsuccessful.
         */
        inline bool success(void) const
        {
            return code_ == Code::OK;
        }

        /**
         * @brief Checks if operation failed.
         * 
         * @return true Operation was unsuccessful.
         * @return false Operation was successful.
         */
        inline bool error(void) const
        {
            return code_ != Code::OK;
        }

        /**
         * @brief Returns the status description.
         * 
         * @return const char* Raw string describing the status.
         */
        inline const char* str(void) const
        {
            /// TODO: need to add memory safe checks
            return CodeTrait::ErrStrLkup[static_cast<size_t>(code_)];
        }

        /**
         * @brief Gets the code value.
         * 
         * @return Code Value.
         */
        inline Code code(void) const { return code_; }

    private:
        Code code_;
};
