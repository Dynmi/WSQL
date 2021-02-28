//
// File:        ix_indexhandle.cc
//
// Description: IX_IndexHandle class implementation
//
// Author:     Haris Wang (dynmiw@gmail.com)
//
// 

#include <iostream>
#include <string.h>
#include "ix.h"
#include <cassert>
using namespace std; 


void IX_FileHdr::print()
{
    printf("\n==========IX_FileHdr=============\n");
    printf("rootPage %d \n", rootPage);
    printf("numPages %d \n", numPages);
    printf("height %d \n", height);
    printf("attrType %d \n", attrType);
    printf("attrLength %d \n", attrLength);
    printf("maxKeys %d \n", maxKeys);
    printf("============IX_FileHdr===========\n\n");
}


//
// create index file with given name on disk, 
// initialize its FileHeader in page 0
// return 0 if success
//
RC CreateIXFile(const char *fileName, 
                AttrType attrType, int attrLength, int pageSize)
{
    // check the attrType & attrLength valid or not
    switch (attrType)
    {
    case INT:{
        if ((unsigned int)attrLength != sizeof(int))
            return IX_FCREATEFAIL;
    }break;
    case FLOAT:{
        if ((unsigned int)attrLength != sizeof(float))
            return IX_FCREATEFAIL;
    }break;
    case STRING:{
        if ((unsigned int)attrLength <= 0 
            || (unsigned int)attrLength > MAXSTRINGLEN)
            return IX_FCREATEFAIL;
    }break;
    default:
        return IX_FCREATEFAIL;
    }

    RC rc = 0;
    PF_FileHandle *pfh = new PF_FileHandle;
    PF_PageHandle *headerPage = new PF_PageHandle;
    char *pData;
    if ((rc = PF_CreateFile(fileName, pageSize))
        || (rc = pfh->OpenFile(fileName))
        || (rc = pfh->AllocatePage(*headerPage))
        || (rc = headerPage->GetData(pData)))
    {
        delete headerPage;
        delete pfh;
        PF_PrintError(rc);
        return rc;
    }
    delete headerPage;

    IX_FileHdr *hdr = new IX_FileHdr;
    hdr->numPages = 1;
    hdr->rootPage = -1;
    hdr->pageSize = pageSize;
    hdr->maxKeys = -1;
    hdr->height = 0;
    hdr->attrType = attrType;
    hdr->attrLength = attrLength;
    memcpy(pData, hdr, sizeof(IX_FileHdr));
    delete hdr;

    if((rc = pfh->MarkDirty(0))
        || (rc = pfh->UnpinPage(0))
        || (rc = pfh->CloseFile()))
    {
        delete pfh;
        PF_PrintError(rc);
        return rc;
    }
    delete pfh;
    return rc;  
}


//
// delete index file with given name on disk
// return 0 if success
//
RC DestroyIXFile(const char *fileName)
{
    RC rc = PF_DestroyFile(fileName);
    if (rc < 0)
    {
        PF_PrintError(rc);
        return IX_PF;
    }
    return rc;
}


void IX_IndexHandle::printNode(PageNum p)
{
    PF_PageHandle ph;
    auto nd = GetNewNode(&ph, p);
    int entrynum = nd->GetNumKeys();
    printf("\n==============%3d===============\n", p);
    printf("printIXNode\n");
    printf("POS  KEY    RID\n");
    for (int i = 0; i < entrynum; i++)
    {
        printf("%-3d ", i);
        auto _key = nd->GetKeyAt(i);
        auto _rid = nd->GetRidAt(i);
        if (hdr.attrType == INT)
        {
            printf("%d", *(int *)_key);
        }else {
            printf("%s", (char *)_key);
        }
        printf("  (%d,%d) \n", _rid->page, _rid->slot);
    }
    printf("================================\n\n");


    for (int i = 0; i < entrynum; i++)
    {
        auto _rid = nd->GetRidAt(i);
        if (_rid->slot == -1)
            printNode(_rid->page);
    }
    delete nd;
    pfh->UnpinPage(p);
}


// Header Page (page 0) and root Page will be pinned
// from OpenIndex() to CloseIndex().

IX_IndexHandle::IX_IndexHandle()
{
    bOpen = false;
    hdrChanged = false;
}


IX_IndexHandle::~IX_IndexHandle()
{
    
}


//
// open given index file, load file header to main memory.
// If this IX file has no node, then create root page.
// return 0 if success
//
RC IX_IndexHandle::OpenIndex(const char *fileName)
{
    if (bOpen)
        return -1;
    pfh = new PF_FileHandle;
    PF_PageHandle ph;
    RC rc;
    // load IX_FileHdr from disk to main memory
    char *pData;
    if ((rc = pfh->OpenFile(fileName))
        || (rc = pfh->GetThisPage(0, ph))
        || (rc = ph.GetData(pData))
        || (rc = pfh->UnpinPage(0)))
    {
        return rc;
    }
    memcpy(&hdr, pData, sizeof(IX_FileHdr));
    if (hdr.rootPage < 0) 
    {
        // create root node by allocating new page
        auto node = GetNewNode(&ph);
        ph.GetPageNum(hdr.rootPage);
        hdr.numPages++;
        hdr.height = 1;
        hdr.maxKeys = node->GetMaxKeys();
        hdrChanged = true;
        pfh->MarkDirty(hdr.rootPage);
        pfh->UnpinPage(hdr.rootPage);
        delete node;
    }

    bOpen = true;
    return rc;
}


//
// close given index file, release the memory it takes, force 
// all the dirty pages to disk
// all pages musted be unpined, otherwise will ERROR
// return 0 if success
//  
RC IX_IndexHandle::CloseIndex()
{
    RC rc;
    if (!bOpen)
        return -1;
    if (hdrChanged)
    {
        PF_PageHandle ph;
        rc = pfh->GetThisPage(0, ph);
        assert(rc == 0);
        char *buf;
        rc = ph.GetData(buf);
        assert(rc == 0);
        memcpy(buf, &hdr, sizeof(hdr));
        rc = pfh->MarkDirty(0);
        assert(rc == 0);
        rc = pfh->UnpinPage(0);
        assert(rc == 0);
        hdrChanged = false;
    }

    rc = pfh->CloseFile();
    assert(rc == 0);
    delete pfh; pfh = NULL;
    bOpen = false;
    return rc;
}


//
// If p = -1,  allocated new page and create new BtreeNode object. 
// If p != -1, load BtreeNode from existing page.
// This page will be pinned in the buffer pool.
// let ph be the pointer to the handle of the new page. 
// return the pointer to the BtreeNode object if success
// return NULL if error
//
BtreeNode* IX_IndexHandle::GetNewNode(PF_PageHandle *ph, PageNum p)
{
    RC rc;
    if (p == -1)
    {
        rc = pfh->AllocatePage(*ph);
        if (rc != 0) return NULL;
        return new BtreeNode(hdr.attrType, hdr.attrLength, *ph, false, hdr.pageSize);
    }else {
        rc = pfh->GetThisPage(p, *ph);
        if (rc != 0) return NULL;
        return new BtreeNode(hdr.attrType, hdr.attrLength, *ph, true, hdr.pageSize);
    }
}


//
// delete BtreeNode object, unpin this page from buffer pool
// return 0 if success
//
RC IX_IndexHandle::DeleteNode(BtreeNode *&node, bool bDirty, PageNum p)
{
    RC rc;
    if (p == -1)
        p = node->GetPageId();

    assert(p > 0);
    if (bDirty)
    {
        rc = pfh->MarkDirty(p);
        assert(rc == 0);
    }

    delete node;
    return pfh->UnpinPage(p);
}


//
// insert given entry(key, rid) to the appropriate leaf node on 
// main memory. If the leaf node is already full, then split it 
// and update its parent recursively. If the old root node is
// splited, then the FileHeader will be updated.
// return 0 if success
// 
RC IX_IndexHandle::InsertEntry(void *key, const RID &rid)
{
    RC rc;
    if (!bOpen) return IX_BADOPEN;
    PF_PageHandle ph;

    BtreeNode *node;
    int *pathPage = new int[hdr.height+1];
    int *pathPos  = new int[hdr.height+1];

    pathPage[0] = hdr.rootPage;

    RID trid;
    int i;
    // travel the path from root node to desired leaf node
    for (i = 0; i < hdr.height - 1; i++)
    {
        node = GetNewNode(&ph, pathPage[i]);
        pathPos[i] = node->FindKey(key);
        
        // if pData is larger than the largest key in this node,
        // update the largest key of this internal node
        if (pathPos[i] == node->GetNumKeys())
        {
            pathPos[i]--;
            node->SetKey(pathPos[i], key);
            pfh->MarkDirty(pathPage[i]);
        }

        // find child node 
        trid = *(node->GetRidAt(pathPos[i]));
        pathPage[i+1] = trid.page;

        pfh->UnpinPage(pathPage[i]);
        delete node;
    }
    
    void *tkey = key;  
    trid = rid;
    int result = 1;
    for (i = hdr.height - 1; i >= 0 && result != 0; i--)
    {
        node = GetNewNode(&ph, pathPage[i]);
        result = node->Insert(tkey, trid);

        if (result != 0) 
        {
            // split this node
            auto newNode = GetNewNode(&ph);
            hdr.numPages++;
            hdrChanged = true;
            node->Split(newNode);
            if (node->FindKey(tkey) < node->GetNumKeys())
            {
                node->Insert(tkey, trid);
            }else {
                newNode->Insert(tkey, trid);
            }

            // (tkey, trid) should be inserted to node's parent
            tkey = node->GetKeyAt(node->GetNumKeys()-1);
            trid = RID(node->GetPageId(), -1);

            if (i > 0)
            {
                // update parent node
                auto parent = GetNewNode(&ph, pathPage[i-1]);
                parent->SetRid(pathPos[i-1], RID(newNode->GetPageId(), -1));
                DeleteNode(parent, 1);
            }else {
                // get new rootNode 
                auto newRoot = GetNewNode(&ph);
                newRoot->Insert(tkey, trid);
                newRoot->Insert(newNode->GetKeyAt(newNode->GetNumKeys()-1),
                                    RID(newNode->GetPageId(), -1));
                hdr.height++;
                hdr.numPages++;
                hdr.rootPage = newRoot->GetPageId();
                hdrChanged = true;
                DeleteNode(newRoot, 1, hdr.rootPage);
            }
            DeleteNode(newNode, 1);
        }
        DeleteNode(node, 1);
    }

    delete pathPage;
    delete pathPos;
    return 0;
}


//
// delete given entry(key, rid) in the index file
// return 0 if success
// return 1 if no such entry
// 
RC IX_IndexHandle::DeleteEntry(void *key, const RID &rid)
{
    RC rc;
    if (!bOpen) return IX_BADOPEN;
    PF_PageHandle ph;

    BtreeNode *node;
    int *pathPage = new int[hdr.height+1];
    int *pathPos  = new int[hdr.height+1];

    pathPage[0] = hdr.rootPage;
    int i;

    // travel the path from root node to desired leaf node
    for (i = 0; i < hdr.height - 1; i++)
    {
        node = GetNewNode(&ph, pathPage[i]);
        pathPos[i] = node->FindKey(key);
        if (pathPos[i] >= node->GetNumKeys())
        {
            DeleteNode(node, 0, pathPage[i]);
            delete pathPage;
            delete pathPos;
            return 1;
            break;
        }
        // find child node 
        RID *_rid = node->GetRidAt(pathPos[i]);
        pathPage[i+1] = _rid->page;

        DeleteNode(node, 0, pathPage[i]);
    }

    node = GetNewNode(&ph, pathPage[i]);
    pathPos[i] = node->FindKey(key);    
    if (pathPos[i] >= node->GetNumKeys())
    {
        DeleteNode(node, 0, pathPage[i]);
        delete pathPage;
        delete pathPos;
        return 1;
    }
    while (1)
    {
        RID *_rid = node->GetRidAt(pathPos[i]);
        if (*_rid == rid)
            break;
        pathPos[i]++;
        if (pathPos[i] == node->GetNumKeys())
        {
            // switch to right neighbor node
            PageNum rightpage = node->GetRight();
            if (rightpage == -1)
                return 1;
            DeleteNode(node, 0);
            pathPos[i] = 0;
            pathPage[i] = rightpage; 
            node = GetNewNode(&ph, pathPage[i]);              
        }
    }
    DeleteNode(node, 0);


    int result = 1;
    for (i = hdr.height - 1; i >= 0 && result != 0; i--)
    {
        if (i > 0)
        {
            node = GetNewNode(&ph, pathPage[i-1]);
            while (node->GetRidAt(pathPos[i-1])->page != pathPage[i])
            {
                pathPos[i-1]++;
                if (pathPos[i-1] == node->GetNumKeys())
                {
                    // switch to right neighbor node
                    PageNum rightpage = node->GetRight();
                    if (rightpage == -1)
                        return 1;
                    DeleteNode(node, 0, pathPage[i]);
                    pathPos[i-1] = 0;
                    pathPage[i-1] = rightpage; 
                    node = GetNewNode(&ph, pathPage[i-1]);              
                }
            }      
            DeleteNode(node, 0, pathPage[i-1]);      
        }

        node = GetNewNode(&ph, pathPage[i]);
        result = node->Remove(key, rid);
        if (i == 0)
        {
            // node is rootNode

            if (result == 0)
            {
                // rootNode not empty after deleteEntry

                RID *_rid = node->GetRidAt(0);
                if (node->GetNumKeys() == 1 && _rid->slot == -1)
                {
                    // change root node
                    PageNum newrootp = _rid->page;
                    DeleteNode(node, 1, hdr.rootPage);
                    pfh->DisposePage(hdr.rootPage);
                    hdr.numPages--;
                    hdr.height--;
                    hdr.rootPage = newrootp;
                    hdrChanged = true;
                    break;                   
                }
            }else {
                
            }
        }else {
            // pathNode[i] is not rootNode

            if (result == 0)
            {
                // pathNode[i] not empty after deleteEntry

                if (pathPos[i] == node->GetNumKeys())
                {
                    auto parent = GetNewNode(&ph, pathPage[i-1]);
                    void *_key = node->GetKeyAt(pathPos[i]-1);
                    parent->SetKey(pathPos[i-1], _key);
                    DeleteNode(parent, 1, pathPage[i-1]);
                    i = i - 2;
                    
                    while (i >= 0)
                    {
                        DeleteNode(node, 1);
                        node = GetNewNode(&ph, pathPage[i]);
                        if (pathPos[i] == node->GetNumKeys()-1)
                        {
                            node->SetKey(pathPos[i], _key);                            
                        }else {
                            break;
                        }
                        i--;
                    }
                }
            }else {
                // node empty after deleteEntry

                // delete node and update its neighbor
                PageNum leftPage = node->GetLeft();
                PageNum rightPage = node->GetRight();
                if (leftPage != -1)
                {
                    auto leftNode = GetNewNode(&ph, leftPage);
                    leftNode->SetRight(rightPage);
                    DeleteNode(leftNode, 1, leftPage);
                }
                if (rightPage != -1)
                {
                    auto rightNode = GetNewNode(&ph, rightPage);
                    rightNode->SetLeft(leftPage);
                    DeleteNode(rightNode, 1, rightPage);             
                }
                DeleteNode(node, 1, pathPage[i]);
                pfh->DisposePage(pathPage[i]);
                hdr.numPages--;
                if (leftPage == -1 && rightPage == -1)
                    hdr.height--;
                hdrChanged = true;
                continue;
            }
        }        
        DeleteNode(node, 1);
    }

    delete pathPage;
    delete pathPos;
    return 0;
}


//
// Forces all pages (along with any contents stored in 
// this class) from the buffer pool to disk.
// 
RC IX_IndexHandle::ForcePages()
{
    if (!bOpen) return IX_BADOPEN;
    if (hdrChanged) 
    {
        char *pData;
        PF_PageHandle ph;
        pfh->GetThisPage(0, ph);
        ph.GetData(pData);
        memcpy(pData, &hdr, sizeof(IX_FileHdr));
        pfh->MarkDirty(0);
    }
    return pfh->ForcePages(ALL_PAGES);
}



RC IX_IndexHandle::SetCurrNode(PageNum page)
{
    if (currNode == NULL)
        return -1;

    PF_PageHandle ph;
    pfh->UnpinPage(currNode->GetPageId());
    delete currNode;
    currNode = GetNewNode(&ph, page);
    return 0;
}


//
// open index scan and initialize currPos and currKey for
// the first GetNextEntry()
// This will navigate to leaf node. 
// return 0 if success
//
RC IX_IndexHandle::OpenScan(CompOp      compOp,
                            void        *value,
                            ClientHint  pinHint)
{
    cmpOp = compOp;
    cmpKey = value;
    PF_PageHandle ph;
    RID *_rid;
    currNode = GetNewNode(&ph, hdr.rootPage);
    while (1)
    {
        currPos = currNode->FindKey(cmpKey);
        if (currPos == currNode->GetNumKeys())
            currPos--;
        _rid = currNode->GetRidAt(currPos);
        if (_rid->slot != -1)
            break;
        SetCurrNode(_rid->page);
    }   
    currKey = currNode->GetKeyAt(currPos);
    return 0;
}


//
// close index scan and release mem it used
// return 0 if success
//
RC IX_IndexHandle::CloseScan()
{
    if (currNode != NULL)
        DeleteNode(currNode, 0);
    currNode = NULL;
    return 0;
}


// 
// start from currPos in currNode, find next match entry
// return 0 if success
// return IX_EOF if end of file
// return -1 if scan not open yet
//
RC IX_IndexHandle::GetNextEntry(RID &rid)
{
    if (currNode == NULL) // scan not open yet
        return -1;
    if (currPos == IX_EOF)
        return IX_EOF;

    // navigate to next match key
    switch (cmpOp)
    {
    case EQ_OP:{ // euqal to cmpKey
        // navigate to next match key
        if (currNode->CmpKey(cmpKey, currKey) < 0)
        {   
            currPos = IX_EOF;
            return IX_EOF;
        }
        while (currNode->CmpKey(cmpKey, currKey) > 0)
        {
            currPos++;
            if (currPos == currNode->GetNumKeys())
            {
                PageNum p = currNode->GetRight();
                if (p == -1) 
                {
                    currPos = IX_EOF;
                    return IX_EOF;
                }else {
                    SetCurrNode(p);
                    currPos = 0;
                }
            }
            currKey = currNode->GetKeyAt(currPos);
        }
        }break;

    case NE_OP:{ // not equal to cmpKey
        // TODO
        }break;

    case LT_OP:{ // less than cmpKey
        // navigate to next match key
        while (!(currNode->CmpKey(cmpKey, currKey) > 0))
        {
            currPos--;
            if (currPos == -1)
            {
                PageNum p = currNode->GetLeft();
                if (p == -1) 
                {
                    currPos = IX_EOF;
                    return IX_EOF;
                }else {
                    SetCurrNode(p);
                    currPos += currNode->GetNumKeys();
                }
            }
            currKey = currNode->GetKeyAt(currPos);
        }
        }break;

    case GT_OP:{ // greater than cmpKey
        // navigate to next match key
        while (!(currNode->CmpKey(cmpKey, currKey) < 0))
        {
            currPos++;
            if (currPos == currNode->GetNumKeys())
            {
                PageNum p = currNode->GetRight();
                if (p == -1) 
                {
                    currPos = IX_EOF;
                    return IX_EOF;
                }else {
                    SetCurrNode(p);
                    currPos = 0;
                }
            }
            currKey = currNode->GetKeyAt(currPos);
        }
        }break;

    case LE_OP:{ // less or equal to cmpKey
        // navigate to next match key
        while (!(currNode->CmpKey(cmpKey, currKey) >= 0))
        {
            currPos--;
            if (currPos == -1)
            {
                PageNum p = currNode->GetLeft();
                if (p == -1) 
                {
                    currPos = IX_EOF;
                    return IX_EOF;
                }else {
                    SetCurrNode(p);
                    currPos += currNode->GetNumKeys();
                }
            }
            currKey = currNode->GetKeyAt(currPos);
        }
        }break;

    case GE_OP:{ // greater or equal to cmpKey
        while (!(currNode->CmpKey(cmpKey, currKey) <= 0))
        {
            currPos++;
            if (currPos == currNode->GetNumKeys())
            {
                PageNum p = currNode->GetRight();
                if (p == -1) 
                {
                    currPos = IX_EOF;
                    return IX_EOF;
                }else {
                    SetCurrNode(p);
                    currPos = 0;
                }
            }
            currKey = currNode->GetKeyAt(currPos);
        }
        }break;

    default:
        // TODO
        break;
    }

    // copy rid associated with matched key
    memcpy(&rid, currNode->GetRidAt(currPos), sizeof(RID));

    // update currPos and currKey for next GetNextEntry()
    if (cmpOp == EQ_OP | cmpOp == GT_OP | cmpOp == GE_OP)
    {
        currPos++;
        if (currPos == currNode->GetNumKeys())
        {
            PageNum p = currNode->GetRight();
            if (p == -1) 
            {
                currPos = IX_EOF;
                return 0;
            }else {
                SetCurrNode(p);
                currPos = 0;
            }
        }
    }else if (cmpOp == LT_OP | cmpOp == LE_OP)
    {
        currPos--;
        if (currPos == -1)
        {
            PageNum p = currNode->GetLeft();
            if (p == -1) 
            {
                currPos = IX_EOF;
                return 0;
            }else {
                SetCurrNode(p);
                currPos += currNode->GetNumKeys();
            }
        }
    }
    currKey = currNode->GetKeyAt(currPos);

    return 0;
}
