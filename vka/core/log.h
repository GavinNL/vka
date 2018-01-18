#ifndef VKA_LOG_H
#define VKA_LOG_H

#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>


static std::string return_current_time_and_date()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    //ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X ");
    ss << std::put_time(std::localtime(&in_time_t), "%X");
    return ss.str();
}

//#define RED std::cout << "\033[1;31m"bold red text\033[0m\n"
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define WHITE   "\033[1;37m"

#define ENDL "\033[0m" << std::endl

#define green(A) GREEN << A << WHITE


#define FUNC_PRINT std::setw(40) << __func__
//#define FUNC_PRINT std::setw(80) << __PRETTY_FUNCTION__

#define INFO if(1 ) std::cout << CYAN   << "INFO (" << return_current_time_and_date() << ") [" << std::left << FUNC_PRINT << "] - "
#define LOG     std::cout << GREEN      << "LOG  (" << return_current_time_and_date() << ") [" << std::left << FUNC_PRINT << "] - "
#define WARN    std::cout << YELLOW     << "WARN (" << return_current_time_and_date() << ") [" << std::left << FUNC_PRINT << "] - "
#define ERROR   std::cout << RED        << "ERROR(" << return_current_time_and_date() << ") [" << std::left << FUNC_PRINT << "] - "


#endif
