#pragma once
#include <vector>
class WMUtility
{
private:
    /* data */
public:
    WMUtility();
    ~WMUtility();
    static std::vector<std::string> column_data; 
    static int column_index;
};

int WMUtility::column_index = 0;
std::vector<std::string> WMUtility::column_data;
