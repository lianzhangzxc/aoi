#include "point.h"
#include <cassert>

namespace aoi
{
    namespace impl
    {
        template<typename TItem, unsigned NodeCapacity>
        QuadTreeNode<TItem, NodeCapacity>::QuadTreeNode(MemBase<TNode>* alloc, ENodeType type, QuadTreeNode* parent, const Rect& bounds)
            : mAlloc(alloc)
            , mParent(parent)
            , mNodeType(type)
            , mBounds(bounds)
            , mItemCount(0)
#ifdef _DEBUG
            , mLevel(0)
#endif
        {
            memset(mChildrens, 0, sizeof(mChildrens));
        }

        template<typename TItem, unsigned NodeCapacity>
        QuadTreeNode<TItem, NodeCapacity>::~QuadTreeNode()
        {

        }

        template<typename TItem, unsigned NodeCapacity>
        bool QuadTreeNode<TItem, NodeCapacity>::Insert(TItem* item)
        {
            Point& pos = item->GetPos();

            if (mNodeType == NodeTypeNormal)
            {
            LABLE_NORMAL:
                int index = mBounds.GetQuadrant(pos) - 1;
                return index >= 0 ? mChildrens[index]->Insert(item) : false;
            }
            else
            {
                if (mItemCount < NodeCapacity)
                {
                    return mBounds.Contains(pos)
                        ? (mItems[mItemCount++] = item, item->mNode = this, true)
                        : false;
                }
                else
                {
                    split();
                    goto LABLE_NORMAL;
                }
            }
        }

        template<typename TItem, unsigned NodeCapacity>
        void QuadTreeNode<TItem, NodeCapacity>::split()
        {
            assert(mNodeType == NodeTypeLeaf);
            mNodeType = NodeTypeNormal;

#ifdef _DEBUG
            printf("split, level:%u, bounds:(left %f, right %f, bottom %f, top %f)\n"
                , mLevel, mBounds.Left(), mBounds.Right(), mBounds.Bottom(), mBounds.Top());
#endif

            // ��һ���ޣ�����
            Rect rect0(mBounds.MidX(), mBounds.Right(), mBounds.MidY(), mBounds.Top());

            // �ڶ����ޣ�����
            Rect rect1(mBounds.Left(), mBounds.MidX(), mBounds.MidY(), mBounds.Top());

            // �������ޣ�����
            Rect rect2(mBounds.Left(), mBounds.MidX(), mBounds.Bottom(), mBounds.MidY());

            // �������ޣ�����
            Rect rect3(mBounds.MidX(), mBounds.Right(), mBounds.Bottom(), mBounds.MidY());

            mChildrens[0] = mAlloc->New(mAlloc, NodeTypeLeaf, this, rect0);
            mChildrens[1] = mAlloc->New(mAlloc, NodeTypeLeaf, this, rect1);
            mChildrens[2] = mAlloc->New(mAlloc, NodeTypeLeaf, this, rect2);
            mChildrens[3] = mAlloc->New(mAlloc, NodeTypeLeaf, this, rect3);

#ifdef _DEBUG
            mChildrens[0]->mLevel = mLevel + 1;
            mChildrens[1]->mLevel = mLevel + 1;
            mChildrens[2]->mLevel = mLevel + 1;
            mChildrens[3]->mLevel = mLevel + 1;
#endif

            for (size_t i = 0; i < mItemCount; i++)
            {
                Point& pos = mItems[i]->GetPos();
                int index = mBounds.GetQuadrant2(pos) - 1;
                mChildrens[index]->Insert(mItems[i]);
            }
            mItemCount = 0;
        }

        template<typename TItem, unsigned NodeCapacity>
        bool QuadTreeNode<TItem, NodeCapacity>::Remove(TItem* item)
        {
            assert(mNodeType == NodeTypeLeaf);
            for (size_t i = 0; i < mItemCount; i++)
            {
                if (mItems[i] == item)
                {
                    unsigned lst = --mItemCount;
                    mItems[i] = mItems[lst];
                    mItems[lst] = nullptr;
                    tryMerge();
                    return true;
                }
            }
            return false;
        }

        template<typename TItem, unsigned NodeCapacity>
        void QuadTreeNode<TItem, NodeCapacity>::tryMerge()
        {
            TNode* node = mParent;
            while (node) {
                assert(node->mNodeType == NodeTypeNormal);
                unsigned count = 0;
                auto& childrens = node->mChildrens;
                for (size_t i = 0; i < ChildrenNum; i++)
                {
                    if (childrens[i]->mNodeType != NodeTypeLeaf) {
                        return;
                    }
                    count += childrens[i]->mItemCount;
                }

                if (count <= NodeCapacity)
                {
#ifdef _DEBUG
                    printf("merge, level:%u, bounds:(left %f, right %f, bottom %f, top %f)\n"
                        , mLevel, mBounds.Left(), mBounds.Right(), mBounds.Bottom(), mBounds.Top());
#endif
                    node->mNodeType = NodeTypeLeaf;
                    node->mItemCount = 0;
                    for (size_t i = 0; i < ChildrenNum; i++)
                    {
                        for (size_t j = 0; j < childrens[i]->mItemCount; j++)
                        {
                            TItem* tempItem = childrens[i]->mItems[j];
                            node->mItems[node->mItemCount++] = tempItem;
                            tempItem->mNode = node;
                        }
                        mAlloc->Delete(childrens[i]);
                    }
                    node = node->mParent;
                }
                else
                {
                    break;
                }
            }
        }

        template<typename TItem, unsigned NodeCapacity>
        void QuadTreeNode<TItem, NodeCapacity>::Query(const Rect& area, TItem*& head, TItem*& tail)
        {
            if (mNodeType == NodeTypeNormal)
            {
                for (size_t i = 0; i < ChildrenNum; i++)
                {
                    if (area.Intersects(mChildrens[i]->mBounds))
                    {
                        mChildrens[i]->Query(area, head, tail);
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < mItemCount; i++)
                {
                    Point& pos = mItems[i]->GetPos();
                    if (area.Contains(pos))
                    {
                        head ? (tail->mNext = mItems[i], tail = mItems[i]) : head = tail = mItems[i];
                    }
                }
            }
        }

        template<typename TItem, unsigned NodeCapacity>
        unsigned QuadTreeNode<TItem, NodeCapacity>::GetItemCount()
        {
            unsigned count = 0;
            if (mNodeType == NodeTypeNormal)
            {
                for (size_t i = 0; i < ChildrenNum; i++)
                {
                    count += mChildrens[i]->GetItemCount();
                }
            }
            else
            {
                count += mItemCount;
            }
            return count;
        }
    }
}