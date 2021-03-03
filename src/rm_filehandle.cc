//
// File:        rm_filehandle.cc
//
// Description: RM_FileHandle class implementation
//
// Author:     Haris Wang (dynmiw@gmail.com)
//
// 
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cmath>
#include <unistd.h>
#include <iostream>
#include "rm.h"

using namespace std;

RC CreateRMFile (const char *fileName, int recordSize, int pageSize, AttrType attrType)
{
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
        return rc;
    }
    delete headerPage;

    RM_FileHdr *hdr = new RM_FileHdr;
    hdr->extRecordSize = recordSize;
    hdr->firstFree = RM_PAGE_LIST_END;
    hdr->numPages = 1; // only header page
    hdr->pageSize = pageSize;
    hdr->attrType = attrType;
    memcpy(pData, hdr, sizeof(RM_FileHdr));
    delete hdr;

    if ((rc = pfh->MarkDirty(0))
        || (rc = pfh->UnpinPage(0))
        || (rc = pfh->CloseFile()))
    {
        printf("CreateRMFile failed \n");
        delete pfh;
        return rc;
    }
    delete pfh;
    return rc;
}


RC DestroyRMFile(const char *fileName)
{
    RC rc = PF_DestroyFile(fileName);
    if (rc != 0)
    {
        printf("Error: pfm.CreateFile... \n");
        return -1;
    }
    return 0;
}


RM_FileHandle::RM_FileHandle()
{
    pfh = NULL;
    bFileOpen = 0;
    bHdrChanged = 0;
}


RM_FileHandle::~RM_FileHandle()
{

}


//
// Open given RMFile
// return 0 if success
//
RC RM_FileHandle::OpenRMFile(const char *fileName)
{
    pfh = new PF_FileHandle;
    RC rc;
    PF_PageHandle ph; // handle for IX header page
    char *pData;
    if ((rc = pfh->OpenFile(fileName))
        || (rc = pfh->GetThisPage(0, ph)) 
        || (rc = ph.GetData(pData))
        || (rc = pfh->UnpinPage(0)))
    {
        return rc;
    }
    bFileOpen = true;
    // load file header to main memory
    memcpy(&hdr, pData, sizeof(RM_FileHdr));
    return rc;
}


//
// Close opened RMFile
// return 0 if success
// return -1 if no opened file
//
RC RM_FileHandle::CloseRMFile()
{
    if (!bFileOpen || pfh == NULL)
        return -1;
    if (bHdrChanged)
    {
        PF_PageHandle ph;
        pfh->GetThisPage(0, ph);
        char *buf;
        ph.GetData(buf);
        memcpy(buf, &hdr, sizeof(hdr));
        pfh->MarkDirty(0);
        pfh->UnpinPage(0);
        bHdrChanged = 0;
    }
    RC rc = pfh->CloseFile();
    delete pfh; pfh = NULL;
    bFileOpen = 0;
    return rc;
}


// persist header into the first page of a file for later
RC RM_FileHandle::SetFileHeader(PF_PageHandle ph) const
{
    char *buf;
    RC rc = ph.GetData(buf);
    memcpy(buf, &hdr, sizeof(hdr));
    return rc;
}


bool RM_FileHandle::hdrChanged() const
{
    return bHdrChanged;
}


PageNum RM_FileHandle::GetNumPages() const
{
    return hdr.numPages;
}


SlotNum RM_FileHandle::GetNumSlots() const
{
    if(hdr.extRecordSize==0)
        return (START_RM_ERR - 3);
    
    int bytes_available = hdr.pageSize - sizeof(RM_PageHdr);
    int slots = floor(1.0 * bytes_available/ (hdr.extRecordSize+1/8));
    int r = sizeof(RM_PageHdr) + bitmap(slots).NumChars;

    while( slots*hdr.extRecordSize > bytes_available-bitmap(slots).NumChars )
    {
        slots--;
    }
    return slots;
}


long long RM_FileHandle::GetNumRecs() const
{
    RC rc;
    long long cnt = 0;
    auto numSlots = this->GetNumSlots();
    PF_PageHandle ph;
    RM_PageHdr pHdr(numSlots);
    PageNum p = (PageNum)-1;
    while (1)
    {
        rc = pfh->GetNextPage(p, ph);
        if (rc == PF_EOF)
            break;
        rc = ph.GetPageNum(p);
        assert(rc == 0);
        if (p != 0)
        {
            rc = this->GetPageHeader(ph, pHdr);
            if (rc != 0) return rc;

            cnt += numSlots - pHdr.numFreeSlots;
        }
        rc = pfh->UnpinPage(p);
        assert(rc == 0);
    }

    return cnt;
}


//
// write RID of all records to 'OutFile'
//
RC RM_FileHandle::WriteAllRids(const char *OutFile) const
{
    RC rc = 0;
    auto numSlots = this->GetNumSlots();
    PF_PageHandle ph;
    RM_PageHdr pHdr(numSlots);
    PageNum p = (PageNum)-1;
    FILE *fpx = fopen(OutFile, "w");
    while (1)
    {
        rc = pfh->GetNextPage(p, ph);
        if (rc == PF_EOF)
            break;
        rc = ph.GetPageNum(p);
        if (rc != 0) return rc;
        if (p > 0)
        {
            rc = this->GetPageHeader(ph, pHdr);
            if (rc != 0) return rc;

            bitmap b(this->GetNumSlots(), pHdr.freeSlotMap);
            for (int s = 0; s < b.getSize(); s++)
            {
                if (b.test(s))
                    fprintf(fpx, "%d %d\n", p, s);
            }
        }
        rc = pfh->UnpinPage(p);
        if (rc != 0) return rc;
    }
    fclose(fpx);

    return rc;
}


RC RM_FileHandle::WriteValue(FILE *&fp, RID rid) const
{
    char *ptr;
    RM_Record rec;
    RC rc = this->GetRec(rid, rec);
    if (rc != 0) 
        return rc;
    rec.GetData(ptr);
    switch (hdr.attrType)
    {
        case STRING:{
            fprintf(fp, "%s ", ptr);
        }break;
        case FLOAT:{
            fprintf(fp, "%f ", *(float *)ptr);
        }break;
        case INT:{
            fprintf(fp, "%d ", *(int *)ptr);           
        }break;
    }
    return rc;
}


//
// GetSlotPointer
//
// Desc: Get slot pointer
// Out:  pData
// Ret:  RM return code
//
RC RM_FileHandle::GetSlotPointer(PF_PageHandle ph, SlotNum s, char*& pData ) const
{
    RC invalid = IsValid();
    if(invalid)
        return invalid;
    
    RC rc = ph.GetData(pData);
    if( rc<0 )
        return rc;
    
    pData += RM_PageHdr(this->GetNumSlots()).size();
    pData += s * hdr.extRecordSize;
    return rc;
}


//
// get next free page in this RM file, set pageNum
// to be the page id of this free page
// return 0 if success
//
RC RM_FileHandle::GetNextFreePage(PageNum& pageNum) 
{
    RC invalid = IsValid(); if(invalid) return invalid; 
    PF_PageHandle ph;
    RM_PageHdr pHdr(this->GetNumSlots());
    
    if(hdr.firstFree != RM_PAGE_LIST_END) 
    {
        // this last page on the free list might actually be full
        RC rc;
        PageNum p;
        if ((rc = pfh->GetThisPage(hdr.firstFree, ph))
            || (rc = ph.GetPageNum(p))
            || (rc = pfh->MarkDirty(p))
            // Needs to be called everytime GetThisPage is called.
            || (rc = pfh->UnpinPage(hdr.firstFree))
            || (rc = this->GetPageHeader(ph, pHdr)))
        return rc;

    }

    if( //we need to allocate a new page
        // because this is the firs time
        hdr.numPages == 0 || 
        hdr.firstFree == RM_PAGE_LIST_END ||
        // or due to a full page
    //      (pHdr.numFreeSlots == 0 && pHdr.nextFree == RM_PAGE_FULLY_USED)
        (pHdr.numFreeSlots == 0)
        ) 
    {
        if(pHdr.nextFree == RM_PAGE_FULLY_USED)
        {
        // std::cerr << "RM_FileHandle::GetNextFreePage - Page Full!" << endl;
        }
        {
        char *pData;

        RC rc;
        if ((rc = pfh->AllocatePage(ph)) ||
            (rc = ph.GetData(pData)) ||
            (rc = ph.GetPageNum(pageNum)))
            return(rc);
        
        // Add page header
        RM_PageHdr phdr(this->GetNumSlots());
        phdr.nextFree = RM_PAGE_LIST_END;
        bitmap b(this->GetNumSlots());
        b.set(); // Initially all slots are free
        b.to_char_buf(phdr.freeSlotMap, b.NumChars);
        // std::cerr << "RM_FileHandle::GetNextFreePage new!!" << b << endl;
        phdr.to_buf(pData);

        // the default behavior of the buffer pool is to pin pages
        // let us make sure that we unpin explicitly after setting
        // things up
        if ((rc = pfh->MarkDirty(pageNum)) 
            || (rc = pfh->UnpinPage(pageNum)))
            return rc;
        }

        // add page to the free list
        hdr.firstFree = pageNum;
        hdr.numPages++;
        assert(hdr.numPages > 1); 
        bHdrChanged = true;
        return 0; // pageNum is set correctly
    }
    // return existing free page
    pageNum = hdr.firstFree;
    return 0;
}


//
// get next free slot in the given page identified by pageNum,
// ph will be set as the PageHandle associated with this page
// return 0 if success
// return -1 if this page if full
//
RC RM_FileHandle::GetNextFreeSlot(PF_PageHandle& ph, PageNum& p, SlotNum& s)
{

    RC rc = IsValid();
    if (rc != 0)
        return rc;

    RM_PageHdr *pHdr = new RM_PageHdr(this->GetNumSlots());

    if ((rc= this->GetNextFreePage(p))
        || (rc = pfh->GetThisPage(p, ph))
        || (rc = this->GetPageHeader(ph, *pHdr))
        || (rc = pfh->UnpinPage(p)))
    {
        delete pHdr;
        return rc;
    }
    bitmap b(this->GetNumSlots(), pHdr->freeSlotMap);
    for (int i = 0; i < this->GetNumSlots(); i++)
    {
        if (!b.test(i)) {
            s = i;
            delete pHdr;
            return 0;
        }
    }
    delete pHdr;
    return -1; // This page is full
}


RC RM_FileHandle::IsValid() const
{
    if((pfh==NULL) || !bFileOpen)
        return (START_RM_ERR - 6);
    if(GetNumSlots()<=0)
        return (START_RM_ERR - 3);
    
    return 0;
}


RC RM_FileHandle::GetPageHeader(PF_PageHandle &ph, RM_PageHdr &pHdr) const
{
  char *buf;
  RC rc = ph.GetData(buf);
  if(rc != 0)
    return rc;
  pHdr.from_buf(buf);
  return rc;
}


RC RM_FileHandle::SetPageHeader(PF_PageHandle &ph, RM_PageHdr &pHdr)
{
  char *buf;
  RC rc = ph.GetData(buf);
  if(rc != 0)
    return rc;
  pHdr.to_buf(buf);
  return 0;
}


//
// get record at given RID position
//
// Desc: Get record
// Out:  rec
// Ret:  RM return code
//
RC RM_FileHandle::GetRec(RID rid, RM_Record &rec) const
{
    if(IsValid())
        return IsValid();
    if(!this->IsValidRID(rid))
        return (START_RM_ERR - 7);

    RC rc;
    PF_PageHandle ph;
    RM_PageHdr pHdr(this->GetNumSlots());
    if((rc = pfh->GetThisPage(rid.page, ph)) 
        || (rc = pfh->UnpinPage(rid.page)))
        return rc;

    GetPageHeader(ph, pHdr);
    bitmap b(this->GetNumSlots(), pHdr.freeSlotMap);
    if(!b.test(rid.slot))
        return (START_RM_WARN + 1);
    char *buf;
    rc = this->GetSlotPointer(ph, rid.slot, buf);
    if (rc != 0)
        return rc;
    rc = rec.Set(buf, hdr.extRecordSize, rid);

    return rc;
}

//
// DeleteRec
//
// Desc: Delete record at given RID position
// Ret:  RM return code
//
RC RM_FileHandle::DeleteRec(const RID &rid)
{
    RC invalid = IsValid();
    if(invalid)
        return invalid;
    if(!this->IsValidRID(rid))
        return (START_RM_ERR - 7);

    RC rc;
    PF_PageHandle ph;
    RM_PageHdr pHdr(this->GetNumSlots());
    if((rc = pfh->GetThisPage(rid.page, ph)) 
        || (rc = pfh->MarkDirty(rid.page))
        || (rc = pfh->UnpinPage(rid.page))
        || (rc = this->GetPageHeader(ph, pHdr)))
    {
        return rc;
    }

    bitmap b(this->GetNumSlots(), pHdr.freeSlotMap);
    if(!b.test(rid.slot))
        return (START_RM_WARN + 1);
    b.set(rid.slot, 0);
    if(pHdr.numFreeSlots == 0)
    {
        // this page used to be full, but now it has free
        // space, so add it to the first list.
        pHdr.nextFree = hdr.firstFree;
        hdr.firstFree = rid.page;
    }
    pHdr.numFreeSlots++;
    b.to_char_buf(pHdr.freeSlotMap, b.NumChars);
    rc = this->SetPageHeader(ph, pHdr);
    return rc;
}

//
// insert record, given record data will be insert to this
// rm file, and rid will be its position.
//
// Desc: Insert record
// Ret:  RM return code
//
RC RM_FileHandle::InsertRec(const void *pData, RID &rid)
{
    if(IsValid())
        return IsValid();

    PF_PageHandle *ph = new PF_PageHandle;
    RM_PageHdr *pHdr = new RM_PageHdr(this->GetNumSlots());
    PageNum p; SlotNum s;
    RC rc;

    if ((rc = this->GetNextFreeSlot(*ph, p, s))
        || (rc = GetPageHeader(*ph, *pHdr)))
    {
        delete ph;
        delete pHdr;
        return rc;
    }
    bitmap b(this->GetNumSlots(), pHdr->freeSlotMap);
    char *pSlot;
    rc = this->GetSlotPointer(*ph, s, pSlot);
    if(rc != 0)
    {
        delete ph;
        delete pHdr;
        return rc;
    }
    rid = RID(p, s);
    if (pData == NULL)
    {
        memset(pSlot, 0x00, hdr.extRecordSize);
    }else {
        memcpy(pSlot, pData, hdr.extRecordSize);
    }

    b.set(s, 1);
    pHdr->numFreeSlots--;
    if(pHdr->numFreeSlots==0)
    {
        hdr.firstFree = pHdr->nextFree;
        pHdr->nextFree = RM_PAGE_FULLY_USED;
    }

    b.to_char_buf(pHdr->freeSlotMap, b.NumChars);
    rc = this->SetPageHeader(*ph, *pHdr);
    delete ph;
    delete pHdr;
    return rc;
}

//
// update given record data
//
// Desc: Update record
// Ret:  RM return code
//
RC RM_FileHandle::UpdateRec(const RM_Record &rec)
{
    if(IsValid())
        return IsValid();

    RID rid;
    rec.GetRid(rid);
    if (!this->IsValidRID(rid))
        return (START_RM_ERR - 7);

    RC rc;
    PF_PageHandle ph;
    RM_PageHdr pHdr(this->GetNumSlots());
    if ((rc = pfh->GetThisPage(rid.page, ph))
        || (rc = pfh->MarkDirty(rid.page))
        || (rc = pfh->UnpinPage(rid.page))
        || (rc = this->GetPageHeader(ph, pHdr)))
        return rc;

    bitmap b(this->GetNumSlots(), pHdr.freeSlotMap);
    if (!b.test(rid.slot))
    {
        b.set(rid.slot, 1);
        b.to_char_buf(pHdr.freeSlotMap, b.NumChars);
        SetPageHeader(ph, pHdr);
    }
    char *pData;
    rc = rec.GetData(pData);
    if (rc != 0)
        return rc;
    char *pSlot;
    rc = this->GetSlotPointer(ph, rid.slot, pSlot);
    if (rc != 0)
        return rc;
    memcpy(pSlot, pData, hdr.extRecordSize);
    return 0;
}

//
// Force a page from the buffer pool to disk. Default value forces 
// all pages.
//
RC RM_FileHandle::ForcePages(PageNum pageNum)
{
    if(IsValid())
        return IsValid();

    if(pageNum==ALL_PAGES | (bFileOpen && pageNum >= 0 && pageNum < hdr.numPages))
        return pfh->ForcePages(pageNum);

    return (START_RM_ERR - 7);
}


// 
// Is it a valid RID
//
bool RM_FileHandle::IsValidRID(const RID rid) const
{
    if(!(rid.page>=0 && rid.page<hdr.numPages && bFileOpen))
        return 0;
    if(!(rid.slot>=0 && rid.slot<this->GetNumSlots()))
        return 0;
    return 1;
}


void RM_FileHandle::print(PageNum p, AttrType type)
{
    RID rid;
    rid.page = p;
    RM_Record rec;
    GetRec(rid, rec);
    char *pData;
    printf("\n==============%3d===============\n", p);
    printf("printRMPage\n");
    printf("SLOT  VALUE\n");
    for (int s = 0; s < GetNumSlots(); s++)
    {
        rid.slot = s;
        if (GetRec(rid, rec) != 0)
            continue;
        rec.GetData(pData);
        printf("%-4d  ", s);
        switch (type)
        {
        case INT:{
            printf("%d", *(int *)pData);
        }break;
        case FLOAT:{
            printf("%f", *(float *)pData);
        }break;
        case STRING:{
            printf("%s", pData);
        }break;        
        default:
            break;
        }
        printf("\n");

    }
    printf("================================\n\n");
}


// 
// expand the pages of RM_File to given number 
// if no such page, create it.
// return 0 if success
//
RC RM_FileHandle::ExpandPage(PageNum numPages)
{
    RC rc = 0;

    auto numS = GetNumSlots();
    PF_PageHandle ph;
    RM_PageHdr pHdr(numS);
    PageNum thisp;
    char *pData;
    while (pfh->GetNumPages() < numPages)
    {
        if ((rc = pfh->AllocatePage(ph))
            || (rc = ph.GetPageNum(thisp))
            || (rc = pfh->UnpinPage(thisp)))
        {
            return rc;
        }
        hdr.numPages++;
        bitmap b(numS);
        b.set();
        b.to_char_buf(pHdr.freeSlotMap, b.NumChars);
        rc = SetPageHeader(ph, pHdr);
        if (rc != 0) return rc;
    }
    bHdrChanged = true;
    return rc;
}


//
// copy from other RM_File with NULL values, 'rhs' must be 
// a RM_File in 'open' status.
// return 0 if success
// return -1 if empty 'rhs'
// 
RC RM_FileHandle::CopyNullFromOthers(RM_FileHandle &rhs)
{
    RC rc = 0;
    RM_Record rec;

    //
    // This function should only works when adding new column
    // in table, in which we need to set the value in this column
    // of existing entries to be NULL. Therefore, the storage layouts 
    // of 'rhs' and this should be the same.
    //
    auto numSlots = rhs.GetNumSlots();
    rc = ExpandPage(rhs.pfh->GetNumPages());
    if (rc != 0) return rc;
    assert(this->GetNumPages() == rhs.GetNumPages());

    PF_PageHandle this_ph;
    PF_PageHandle rhs_ph;
    RM_PageHdr rhs_pHdr(numSlots);
    PageNum p = (PageNum)-1;
    char *ptr;
    while (1)
    {
        rc = this->pfh->GetNextPage(p, this_ph);
        if (rc == PF_EOF)
            break;
        rc = this_ph.GetPageNum(p);
        assert(rc == 0);

        rc = rhs.PinPage(rhs_ph, p);
        assert(rc == 0);

        if (p != 0)
        {
            rc = rhs.GetPageHeader(rhs_ph, rhs_pHdr);
            if (rc != 0) return rc;
            bitmap b(numSlots, rhs_pHdr.freeSlotMap);

            for (int s = 0; s < numSlots; s++)
            {
                if (!b.test(s))
                    continue;
                rhs.GetSlotPointer(rhs_ph, s, ptr);
                rec.Set(ptr, hdr.extRecordSize, RID(p, s));
                rc = this->UpdateRec(rec);
                assert(rc == 0);
            }
        }

        rc = rhs.UnpinPage(p);
        assert(rc == 0);

        rc = this->pfh->UnpinPage(p);
        assert(rc == 0);
    }
    return 0;
}


RC RM_FileHandle::PinPage(PF_PageHandle &ph, PageNum p)
{
    return pfh->GetThisPage(p, ph);
}


RC RM_FileHandle::UnpinPage(PageNum p)
{
    return pfh->UnpinPage(p);
}
