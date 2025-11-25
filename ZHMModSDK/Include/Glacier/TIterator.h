#pragma once

template <class T>
class TIterator {
protected:
    TIterator(T* p_Current) :
        m_pCurrent(p_Current) {
    }

public:
    T* m_pCurrent;
};