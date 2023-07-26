#pragma once
#include <vector>
class WMUtility
{
private:
    /* data */
public:
    WMUtility();
    ~WMUtility();
    static std::vector<std::string> chinamobile_column_data; 
    static std::vector<std::string> unicom_column_data; 
    static std::vector<std::string> default_ipv4_column_data; 
    static std::vector<std::string> default_ipv6_column_data; 
};

