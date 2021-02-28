//
// File:        btree_node.h
//
// Description: Interface for btree node
//
// Author:     Haris Wang (dynmiw@gmail.com)
//

#include "wsql.h"
#include "pf.h"
#include "rm_rid.h"
#include "ix_error.h"


class BtreeNode {
private:
    char     *keys;
    RID      *rids;
    int      numKeys;
    AttrType attrType;
    int      attrLength;
    PageNum  pageId;
    int      maxKeys;

public:
    BtreeNode(AttrType _attrType, int _attrLength,
                PF_PageHandle& ph, bool fromDisk, int pageSize);
    ~BtreeNode();

    int GetPageId() const;
    int GetMaxKeys() const;
    int GetNumKeys() const;
    PageNum GetLeft() const;
    PageNum GetRight() const;
    int SetNumKeys(int n);
    int SetLeft(PageNum p);
    int SetRight(PageNum p);
    int SetKey(int pos, const void *newKey);
    int SetRid(int pos, const RID rid);
    void* GetKeyAt(int pos) const;
    RID*  GetRidAt(int pos) const;

    int CmpKey(void *a, void *b) const;
    int FindKey(void *key) const;

    int Insert(void *newKey, const RID& newRid, int pos = -1);
    int Remove(void *Key, const RID& Rid);
    int Remove(int pos);

    RC  Split(BtreeNode *rhs);
    RC  Merge(BtreeNode *rhs);
};
