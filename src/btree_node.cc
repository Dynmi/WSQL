//
// File:        btree_node.cc
//
// Description: Implementation for B-link tree node
//
// Author:     Haris Wang (dynmiw@gmail.com)
//

#include <iostream>
#include <string.h>
#include <assert.h>
#include "btree_node.h"


// 
// the page musted be pinned before any call to BtreeNode 
//
BtreeNode::BtreeNode(AttrType _attrType, int _attrLength,
                PF_PageHandle& ph, bool fromDisk = true, int pageSize = 4092)
{
    maxKeys = (pageSize - sizeof(numKeys) - 2*sizeof(PageNum))
                / (sizeof(RID)+_attrLength); 
    
    char *pData;
    ph.GetData(pData);
    ph.GetPageNum(pageId);
    attrType = _attrType;
    attrLength = _attrLength;
    keys = pData;
    rids = (RID*) (pData + _attrLength * maxKeys);

    //
    // Page Layout
    //  maxKeys * key - takes up maxKeys * attrLength
    //  maxKeys * RID - takes up maxKeys * sizeof(RID)
    //  numKeys - takes up sizeof(int)
    //  left    - takes up sizeof(PageNum)
    //  right   - takes up sizeof(PageNum)
    //

    if (fromDisk)
    {
        numKeys = *(int *)(rids + maxKeys);
    }else {
        SetNumKeys(0);
        SetLeft(-1);
        SetRight(-1);
    }
    
}

BtreeNode::~BtreeNode()
{

}


int BtreeNode::GetNumKeys() const
{
    return numKeys;
}


int BtreeNode::GetMaxKeys() const
{
    return maxKeys;
}


int BtreeNode::GetPageId() const
{
    return pageId;
}


// 
// set new NumKeys to page and local var
// return 0 if success
//
int BtreeNode::SetNumKeys(int n)
{
    assert(n >= 0 | n < maxKeys);

    memcpy((char *)rids + maxKeys * sizeof(RID),
            &n, sizeof(int));
    numKeys = n;
    return 0;    
}


PageNum BtreeNode::GetLeft() const
{
    PageNum *loc = (PageNum *)((char *)rids + maxKeys * sizeof(RID)
                    + sizeof(int));
    return *loc;
}


int BtreeNode::SetLeft(PageNum p)
{
    memcpy((char *)rids + maxKeys * sizeof(RID) + sizeof(int),
            &p, sizeof(PageNum));
    return 0;    
}


int BtreeNode::GetRight() const
{
    PageNum *loc = (PageNum *)((char *)rids + maxKeys * sizeof(RID)
                    + sizeof(int) + sizeof(PageNum));
    return *loc;
}


int BtreeNode::SetRight(PageNum p)
{
    memcpy((char *)rids + maxKeys * sizeof(RID) + sizeof(int) + sizeof(PageNum),
            &p, sizeof(PageNum));
    return 0;    
}


void *BtreeNode::GetKeyAt(int pos) const
{
    assert(pos >= 0 && pos < numKeys);
    return (void *)(keys + attrLength * pos);
}


RID *BtreeNode::GetRidAt(int pos) const
{
    assert(pos >= 0 && pos < numKeys);
    return (rids+pos);
}


//
// write newKey to page at given position
// return 0 if success
//
int BtreeNode::SetKey(int pos, const void* newKey)
{
    assert(pos >= 0 | pos < maxKeys);
    void *loc = keys + pos * attrLength;
    memcpy(loc, newKey, attrLength);
    return 0;
}


int BtreeNode::SetRid(int pos, const RID rid)
{
    assert(pos >= 0 | pos < maxKeys);
    memcpy(rids+pos, &rid, sizeof(RID));
    return 0;
}


//
// compare key a and key b
// return 1 if a > b
// return 0 if a = b
// return -1 if a < b
//
int BtreeNode::CmpKey(void *a, void *b) const
{
    switch (attrType)
    {
    case STRING:{
        return strcmp((char *)a, (char *)b);
        }break;
    case FLOAT:{
        float fa = *(float *)a;
        float fb = *(float *)b;
        if (fa >  fb) return 1;
        if (fa == fb) return 0;
        if (fa <  fb) return -1;
        }break;
    case INT:{
        int fa = *(int *)a;
        int fb = *(int *)b;
        if (fa >  fb) return 1;
        if (fa == fb) return 0;
        if (fa <  fb) return -1;
        }break;
    }

    return 0;
}


//
// return desired position of given key in internal node
// return the left most position of given key in leaf node
//
int BtreeNode::FindKey(void *key) const
{
    int i = 0;
    while (i < numKeys)
    {
        if (CmpKey(key, (void *)(keys + attrLength * i)) <= 0)
            break;
        i++;
    }
    return i;
}


//
// insert given pair (key, rid) to this node
// return 0 if success
// return -1 if no space
//
int BtreeNode::Insert(void* newKey, const RID& newRid, int pos)
{
    if (numKeys >= maxKeys)
        return -1;

    if (pos == -1) 
        pos = FindKey(newKey);

    if (pos < numKeys)
    {
        for (int p = attrLength * numKeys - 1; 
                p >= attrLength * pos; p--)
        {
            keys[p+attrLength] = keys[p];
        }
        for (int p = numKeys; p > pos; p--)
        {
            rids[p] = rids[p-1];
        }
    }

    SetKey(pos, newKey);
    SetRid(pos, newRid);
    SetNumKeys(numKeys+1);
    return 0;
}


//
// remove given pair (key, rid) from this node
// return 0 if success
// return -1 if empty node after Remove
// return -2 if there is no such key
// 
int BtreeNode::Remove(void *Key, const RID& Rid)
{
    int pos;
    RID *_rid;
    for (pos = FindKey(Key); pos < numKeys; pos++)
    {
        _rid = this->GetRidAt(pos);
        if (*_rid == Rid)
            break;
    }
    return this->Remove(pos);
}


//
// remove pair at given position from this node
// return 0 if success
// return -1 if empty node after Remove
// return -2 if there is no such key
// 
int BtreeNode::Remove(int pos)
{
    if (pos >= numKeys)
    {
        return -2;
    }else {
        for (int p = attrLength * pos; p <= attrLength * (numKeys - 1) - 1; p++)
            keys[p] = keys[p + attrLength];
        for (int p = pos; p < numKeys - 1; p++)
            rids[p] = rids[p + 1];
    }
    
    SetNumKeys(GetNumKeys() - 1);
    if (numKeys == 0) 
    {
        return -1;
    }else {
        return 0;
    }
}


//
// split this node, shift higher keys to rhs which
// will be its right neighbor
// return 0 if success
// return -1 if error
//
RC BtreeNode::Split(BtreeNode *rhs)
{
    if (numKeys < maxKeys)
        return -1;
    
    int StMovedPos = (numKeys + 1) / 2;
    int MovedCount = numKeys - StMovedPos;
    int rhsNumKeys = rhs->GetNumKeys();
    if (rhsNumKeys + MovedCount > rhs->GetMaxKeys())
        return -1;
    
    memcpy((void *)(rhs->keys + rhsNumKeys * attrLength),
            (void *)(keys + StMovedPos * attrLength),
            MovedCount * attrLength);
    memcpy((void *)(rhs->rids + rhsNumKeys),
            (void *)(rids + StMovedPos),
            MovedCount * sizeof(RID));
    this->SetNumKeys(StMovedPos);
    rhs->SetNumKeys(rhsNumKeys + MovedCount);

    rhs->SetRight(this->GetRight());
    rhs->SetLeft(this->pageId);
    this->SetRight(rhs->pageId);
    return 0;
}


// 
// merge rhs into this node, rhs is a right neighbour
// return 0 if success
// return -1 if error
//
RC BtreeNode::Merge(BtreeNode *rhs)
{
    int rhsNumKeys = rhs->GetNumKeys();
    if (rhsNumKeys + numKeys > maxKeys)
        return -1;
    
    memcpy((void *)(keys + numKeys * attrLength),
            (void *)rhs->keys, rhsNumKeys * attrLength);
    memcpy((void *)(rids + numKeys),
            (void *)rhs->rids, rhsNumKeys);
    SetNumKeys(numKeys + rhsNumKeys);

    if (rhs->GetLeft() == this->pageId)
    {
        this->SetRight(rhs->GetRight());
    }else {
        this->SetLeft(rhs->GetLeft());
    }

    return 0;
}
