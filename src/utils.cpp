#include <utils.hpp>

#include <sstream>

using namespace Cass;

const char* ComException::what() const {
    std::ostringstream ss;
    ss << "Faliure, HRESULT : " << std::hex << result << '\n';
    std::string str = ss.str();
    return str.c_str();
}