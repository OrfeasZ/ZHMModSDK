#pragma once

#include "TPair.h"

template <typename T>
class TBinaryTreeNode
{
public:
    static TBinaryTreeNode* GetNextNode(TBinaryTreeNode* pNode)
    {
        TBinaryTreeNode* result = pNode->m_pRight;

        if (result)
        {
            for (TBinaryTreeNode* i = result->m_pLeft; i; i = i->m_pLeft)
            {
                result = i;
            }
        }
        else
        {
            result = pNode->m_pParent;

            if (result)
            {
                if (result->m_pLeft == pNode)
                {
                    return result;
                }

                do
                {
                    if (result->m_pLeft == pNode)
                    {
                        break;
                    }

                    pNode = result;
                    result = result->m_pParent;
                }
                while (result);
            }

            result = pNode->m_pParent;

            if (!result)
            {
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
class TBinaryTreeIterator
{
public:
    inline TBinaryTreeIterator() :
        m_pCurrent(nullptr)
    {

    }

    inline TBinaryTreeIterator(T* ptr) :
        m_pCurrent(ptr)
    {
    }

    inline TBinaryTreeIterator<T>& operator++()
    {
        TBinaryTreeNode<T>* node = reinterpret_cast<TBinaryTreeNode<T>*>(reinterpret_cast<char*>(this->m_pCurrent) - offsetof(TBinaryTreeNode<T>, TBinaryTreeNode<T>::m_data));

        this->m_pCurrent = &TBinaryTreeNode<T>::GetNextNode(node)->m_data;

        return *this;
    }

    inline bool operator==(const TBinaryTreeIterator<T>& other) const
    {
        return m_pCurrent == other.m_pCurrent;
    }

    inline bool operator!=(const TBinaryTreeIterator<T>& other) const
    {
        return m_pCurrent != other.m_pCurrent;
    }

    inline T* operator*() const
    {
        return m_pCurrent;
    }

    inline T* operator->() const
    {
        return m_pCurrent;
    }

public:
    T* m_pCurrent;
};

template <typename T>
class TBinaryTree
{
public:
    struct SFakeTreeNode
    {
        int m_nReserved; // 0x00
        TBinaryTreeNode<T>* m_pNULL; // 0x08
        TBinaryTreeNode<T>* m_pRightRoot; // 0x10
        TBinaryTreeNode<T>* m_pLeftRoot; // 0x18
    };

    typedef TBinaryTreeIterator<T> iterator;
    typedef const TBinaryTreeIterator<T> const_iterator;

public:
    inline iterator end()
    {
        return reinterpret_cast<T*>(&m_nSize);
    }

    inline iterator begin()
    {
        iterator result;

        if (m_tree.m_pLeftRoot)
        {
            result.m_pCurrent = &TBinaryTreeNode<T>::GetNextNode(reinterpret_cast<TBinaryTreeNode<T>*>(this))->m_data;
        }
        else
        {
            result.m_pCurrent = reinterpret_cast<T*>(&m_nSize);
        }

        return result;
    }

public:
    SFakeTreeNode m_tree; // 0x00
    int m_nSize; // 0x20
};

template <typename T>
class TRedBlackTree :
    public TBinaryTree<T>
{
};

template <typename T, typename Z>
class TMap
{
public:
    typedef TPair<T, Z> value_type;

    typedef TBinaryTreeIterator<value_type> iterator;
    typedef const TBinaryTreeIterator<value_type> const_iterator;

public:
    inline iterator end()
    {
        return m_container.end();
    }

    inline iterator begin()
    {
        return m_container.begin();
    }

    inline iterator find(const T& key)
    {
        TBinaryTreeNode<value_type>* node = find(m_container.m_tree.m_pLeftRoot, key);

        if (node)
        {
            return &node->m_data;
        }

        return end();
    }

    inline TBinaryTreeNode<value_type>* find(TBinaryTreeNode<value_type>* root, const T& key)
    {
        if (!root || root->m_data.first == key)
        {
            return root;
        }

        if (root->m_data.first < key)
        {
            return find(root->m_pRight, key);
        }

        return find(root->m_pLeft, key);
    }

    inline size_t size() const
    {
        return m_container.m_nSize;
    }

protected:
    TRedBlackTree<value_type> m_container;
};
