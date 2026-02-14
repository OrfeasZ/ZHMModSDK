#pragma once

#include "TIterator.h"

template <typename T>
class TBinaryTreeNode {
public:
    static TBinaryTreeNode* GetNextNode(TBinaryTreeNode* p_Node) {
        // Case 1: node has right child -> go to leftmost node in right subtree
        TBinaryTreeNode* s_CurrentNode = p_Node;
        if (s_CurrentNode->m_pRight) {
            TBinaryTreeNode* s_SuccessorNode = s_CurrentNode->m_pRight;

            while (s_SuccessorNode->m_pLeft) {
                s_SuccessorNode = s_SuccessorNode->m_pLeft;
            }

            return s_SuccessorNode;
        }

        // Case 2: no right child -> go up until the current node is a left child
        TBinaryTreeNode* s_ParentNode = s_CurrentNode->m_pParent;

        if (!s_ParentNode) {
            return s_CurrentNode;
        }

        while (s_ParentNode && s_CurrentNode != s_ParentNode->m_pLeft) {
            s_CurrentNode = s_ParentNode;
            s_ParentNode = s_ParentNode->m_pParent;
        }

        return s_ParentNode;
    }

public:
    int m_nBalance;
    TBinaryTreeNode<T>* m_pParent;
    TBinaryTreeNode<T>* m_pLeft;
    TBinaryTreeNode<T>* m_pRight;
    T m_data;
};

template <typename T>
class TBinaryTreeIterator : public TIterator<T> {
public:
    TBinaryTreeIterator()
        : TIterator<T>(nullptr)
    {
    }

    explicit TBinaryTreeIterator(T* ptr)
        : TIterator<T>(ptr)
    {
    }

    TBinaryTreeIterator<T>& operator++() {
        using NodeType = TBinaryTreeNode<std::remove_const_t<T>>;

        const NodeType* s_Node = reinterpret_cast<const NodeType*>(
            reinterpret_cast<const char*>(this->m_pCurrent) - offsetof(NodeType, m_data)
        );

        NodeType* s_Next = NodeType::GetNextNode(const_cast<NodeType*>(s_Node));
        this->m_pCurrent = &s_Next->m_data;

        return *this;
    }

    bool operator==(const TBinaryTreeIterator<T>& other) const {
        return this->m_pCurrent == other.m_pCurrent;
    }

    bool operator!=(const TBinaryTreeIterator<T>& other) const {
        return this->m_pCurrent != other.m_pCurrent;
    }

    T& operator*() const {
        return *this->m_pCurrent;
    }

    T* operator->() const {
        return this->m_pCurrent;
    }
};

template <typename T>
class TBinaryTree {
public:
    struct SFakeTreeNode {
        int m_nReserved;
        TBinaryTreeNode<T>* m_pNULL;
        TBinaryTreeNode<T>* m_pRightRoot;
        TBinaryTreeNode<T>* m_pLeftRoot;
    };

    using iterator = TBinaryTreeIterator<T>;
    using const_iterator = TBinaryTreeIterator<const T>;

public:
    const_iterator begin() const {
        if (m_tree.m_pLeftRoot) {
            using NodeType = TBinaryTreeNode<std::remove_const_t<T>>;

            NodeType* s_FakeRootNode = reinterpret_cast<NodeType*>(const_cast<TBinaryTree<T>*>(this));
            NodeType* s_FirstNode = NodeType::GetNextNode(s_FakeRootNode);

            return const_iterator(&s_FirstNode->m_data);
        }

        return const_iterator(reinterpret_cast<const T*>(&m_nSize));
    }

    iterator begin() {
        if (m_tree.m_pLeftRoot) {
            using NodeType = TBinaryTreeNode<std::remove_const_t<T>>;

            NodeType* s_FakeRootNode = reinterpret_cast<NodeType*>(this);
            NodeType* s_FirstNode = NodeType::GetNextNode(s_FakeRootNode);

            return iterator(&s_FirstNode->m_data);
        }

        return iterator(reinterpret_cast<T*>(&m_nSize));
    }

    const_iterator end() const {
        return const_iterator(reinterpret_cast<const T*>(&m_nSize));
    }

    iterator end() {
        return iterator(reinterpret_cast<T*>(&m_nSize));
    }

    size_t size() const {
        return m_nSize;
    }

    bool contains(const T& p_Value) const {
        return find(p_Value) != end();
    }

    const_iterator find(const T& p_Value) const {
        auto* s_Node = find(m_tree.m_pLeftRoot, p_Value);

        return s_Node ? const_iterator(&s_Node->m_data) : end();
    }

    iterator find(const T& p_Value) {
        auto* s_Node = find(m_tree.m_pLeftRoot, p_Value);

        return s_Node ? iterator(&s_Node->m_data) : end();
    }

private:
    TBinaryTreeNode<T>* find(TBinaryTreeNode<T>* p_Root, const T& p_Value) const {
        if (!p_Root || p_Root->m_data == p_Value) {
            return p_Root;
        }

        if (p_Root->m_data < p_Value) {
            return find(p_Root->m_pRight, p_Value);
        }

        return find(p_Root->m_pLeft, p_Value);
    }

public:
    SFakeTreeNode m_tree;
    int m_nSize;
};