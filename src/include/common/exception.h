//
// Created by huan.yang on 2026-01-28.
//

#pragma once
#include <exception>
#include <string>
namespace YourSQL {

    enum class ExceptionType {

    };

    class Exception : public std::exception {
    public:
        Exception(std::string message);
        Exception(ExceptionType type, std::string message);

        ExceptionType type;

        const char * what() const noexcept override;

    protected:
        void format(va_list ap);

    private:
        std::string exceptionMessage;
    };

}
