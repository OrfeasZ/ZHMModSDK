#pragma once

#include "TPair.h"
#include "TRedBlackTree.h"

template <typename T, typename Z>
class TMap {
public:
    typedef TPair<T, Z> value_type;

    typedef TBinaryTreeIterator<value_type> iterator;
    typedef const TBinaryTreeIterator<value_type> const_iterator;

public:
    inline iterator end() {
        return m_container.end();
    }

    inline iterator begin() {
        return m_container.begin();
    }

    inline iterator find(const T& key) {
        TBinaryTreeNode<value_type>* node = find(m_container.m_tree.m_pLeftRoot, key);

        if (node) {
            return &node->m_data;
        }

        return end();
    }

    inline TBinaryTreeNode<value_type>* find(TBinaryTreeNode<value_type>* root, const T& key) {
        if (!root || root->m_data.first == key) {
            return root;
        }

        if (root->m_data.first < key) {
            return find(root->m_pRight, key);
        }

        return find(root->m_pLeft, key);
    }

    inline size_t size() const {
        return m_container.m_nSize;
    }

protected:
    TRedBlackTree<value_type> m_container;
};