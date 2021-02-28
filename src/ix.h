//
// File:    ix.h
// 
// Description: Interface for IX component
//
// Author:     Haris Wang (dynmiw@gmail.com)
//

#ifndef IX_H
#define IX_H

#include "wsql.h"
#include "rm_rid.h"
#include "pf.h"
#include "btree_node.h"
#include "ix_error.h"


struct IX_FileHdr {
    int rootPage;
    int numPages;
    int height;
    int pageSize;
    AttrType attrType;
    int attrLength;
    int maxKeys;

    void print();
};



RC CreateIXFile(const char *fileName,
                AttrType attrType, int attrLength, int pageSize = 4092);

RC DestroyIXFile(const char *fileName);


class IX_IndexHandle {
private:
    PF_FileHandle *pfh;
    bool bOpen;
    bool hdrChanged;

    // ################## index scan ################### //
    BtreeNode *currNode;
    int  currPos;
    void *currKey;
    CompOp cmpOp;
    void   *cmpKey;
public:
    IX_FileHdr hdr;

    IX_IndexHandle();                             // Constructor
    ~IX_IndexHandle();                             // Destructor
    RC OpenIndex(const char *fileName);
    RC CloseIndex();
    BtreeNode* GetNewNode(PF_PageHandle *ph, PageNum p = -1);
    RC DeleteNode(BtreeNode *&node, bool bDirty, PageNum p = -1);

    // ################## index modification ################### //
    RC InsertEntry     (void *key, const RID &rid);  // Insert new index entry
    RC DeleteEntry     (void *key, const RID &rid);  // Delete index entry
    RC ForcePages      ();                             // Copy index to disk

    // ################## index scan ################### //
    RC SetCurrNode(PageNum page);
    RC OpenScan      (CompOp      compOp, // Initialize index scan
                      void        *value,
                      ClientHint  pinHint = NO_HINT);           
    RC GetNextEntry  (RID &rid);                         // Get next matching entry
    RC CloseScan     ();                                 // Terminate index scan


    void printNode(PageNum p);
};


#endif