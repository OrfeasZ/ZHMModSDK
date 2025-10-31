#pragma once

template <typename T>
class TBinaryTreeNode {
public:
    static TBinaryTreeNode* GetNextNode(TBinaryTreeNode* pNode) {
        TBinaryTreeNode* result = pNode->m_pRight;

        if (result) {
            for (TBinaryTreeNode* i = result->m_pLeft; i; i = i->m_pLeft) {
                result = i;
            }
        }
        else {
            result = pNode->m_pParent;

            if (result) {
                if (result->m_pLeft == pNode) {
                    return result;
                }

                do {
                    if (result->m_pLeft == pNode) {
                        break;
                    }

                    pNode = result;
                    result = result->m_pParent;
                } while (result);
            }

            result = pNode->m_pParent;

            if (!result) {
                return pNode;
            }
        }

        return result;
    }

public:
    int m_nBalance; // 0x00
    TBinaryTreeNode<T>* m_pParent; // 0x08
    TBinaryTreeNode<T>* m_pLeft; // 0x10
    TBinaryTreeNode<T>* m_pRight; // 0x18
    T m_data; // 0x20
};

template <typename T>
class TBinaryTreeIterator {
public:
    TBinaryTreeIterator() :
        m_pCurrent(nullptr) {
    }

    TBinaryTreeIterator(T* ptr) :
        m_pCurrent(ptr) {
    }

    TBinaryTreeIterator<T>& operator++() {
        TBinaryTreeNode<T>* node = reinterpret_cast<TBinaryTreeNode<T>*>(reinterpret_cast<char*>(this->m_pCurrent) -
            offsetof(TBinaryTreeNode<T>, TBinaryTreeNode<T>::m_data));

        this->m_pCurrent = &TBinaryTreeNode<T>::GetNextNode(node)->m_data;

        return *this;
    }

    bool operator==(const TBinaryTreeIterator<T>& other) const {
        return m_pCurrent == other.m_pCurrent;
    }

    bool operator!=(const TBinaryTreeIterator<T>& other) const {
        return m_pCurrent != other.m_pCurrent;
    }

    T& operator*() const {
        return *m_pCurrent;
    }

    T* operator->() const {
        return m_pCurrent;
    }

public:
    T* m_pCurrent;
};

template <typename T>
class TBinaryTree {
public:
    struct SFakeTreeNode {
        int m_nReserved; // 0x00
        TBinaryTreeNode<T>* m_pNULL; // 0x08
        TBinaryTreeNode<T>* m_pRightRoot; // 0x10
        TBinaryTreeNode<T>* m_pLeftRoot; // 0x18
    };

    typedef TBinaryTreeIterator<T> iterator;
    typedef const TBinaryTreeIterator<T> const_iterator;

public:
    iterator end() {
        return reinterpret_cast<T*>(&m_nSize);
    }

    iterator begin() {
        iterator result;

        if (m_tree.m_pLeftRoot) {
            result.m_pCurrent = &TBinaryTreeNode<T>::GetNextNode(reinterpret_cast<TBinaryTreeNode<T>*>(this))->m_data;
        }
        else {
            result.m_pCurrent = reinterpret_cast<T*>(&m_nSize);
        }

        return result;
    }

public:
    SFakeTreeNode m_tree; // 0x00
    int m_nSize; // 0x20
};