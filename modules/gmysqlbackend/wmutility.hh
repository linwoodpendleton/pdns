#pragma once
class WMUtility
{
private:
    /* data */
public:
    WMUtility();
    ~WMUtility();
    static char* column_data[30];
    static int column_index;
};
int WMUtility::column_index = 0;
char* WMUtility::column_data[30] = {nullptr};

