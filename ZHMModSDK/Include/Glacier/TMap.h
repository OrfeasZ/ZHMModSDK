#pragma once

#include "TPair.h"
#include "TRedBlackTree.h"

template <typename TKeyType, typename TValueType>
class TMap {
public:
    using value_type = TPair<TKeyType, TValueType>;

    using iterator = TBinaryTreeIterator<value_type>;
    using const_iterator = TBinaryTreeIterator<const value_type>;

public:
    const_iterator begin() const {
        return m_container.begin();
    }

    iterator begin() {
        return m_container.begin();
    }

    const_iterator end() const {
        return m_container.end();
    }

    iterator end() {
        return m_container.end();
    }

    bool contains(const TKeyType& p_Key) const {
        return find(p_Key) != end();
    }

    const_iterator find(const TKeyType& p_Key) const {
        auto* s_Node = find(m_container.m_tree.m_pLeftRoot, p_Key);

        return s_Node ? const_iterator(&s_Node->m_data) : end();
    }

    iterator find(const TKeyType& p_Key) {
        auto* s_Node = find(m_container.m_tree.m_pLeftRoot, p_Key);

        return s_Node ? iterator(&s_Node->m_data) : end();
    }

    size_t size() const {
        return m_container.m_nSize;
    }

protected:
    TBinaryTreeNode<value_type>* find(TBinaryTreeNode<value_type>* p_Root, const TKeyType& p_Key) {
        if (!p_Root || p_Root->m_data.first == p_Key) {
            return p_Root;
        }

        if (p_Root->m_data.first < p_Key) {
            return find(p_Root->m_pRight, p_Key);
        }

        return find(p_Root->m_pLeft, p_Key);
    }

    TRedBlackTree<value_type> m_container;
};