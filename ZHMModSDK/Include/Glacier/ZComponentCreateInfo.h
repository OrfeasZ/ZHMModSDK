#pragma once

class ZComponentCreateInfo {
public:
    struct SArgumentInfo {
        STypeID* m_type;
        const void* m_pData;
    };

    int m_nCount;
    SArgumentInfo m_aArguments[5];
};