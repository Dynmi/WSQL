//
// File:        rm.h
//
// Description: Record Manager component interface
// This file does not include the interface for the RID class.  This is
// found in rm_rid.h
//
// Author:     Haris Wang (dynmiw@gmail.com)
//
// 
#ifndef RM_H
#define RM_H

// Please DO NOT include any files other than wsql.h and pf.h in this
// file.  When you submit your code, the test program will be compiled
// with your rm.h and your wsql.h, along with the standard pf.h that
// was given to you.  Your rm.h, your wsql.h, and the standard pf.h
// should therefore be self-contained (i.e., should not depend upon
// declarations in any other file).

// Do not change the following includes
#include <climits>
#include "wsql.h"
#include "rm_rid.h"
#include "pf.h"


//
// bitmap: provides bitmap structure for each page
// for each bit, 0 stands for "free" bit, 1 stands for "used"
//
class bitmap {
private:
    unsigned int size;
    char* buffer;
public:
    unsigned int NumChars;

    bitmap(int numBits, char *buf=NULL);
    ~bitmap();

    void set(unsigned int pos = UINT_MAX, bool sign = 0);
    bool test(unsigned int bitNumber) const;

    RC to_char_buf(char * buf, int len) const;
    RC getSize() const;
};


//
// RM_FileHdr: Header structure for RM file
//
struct RM_FileHdr
{
    int firstFree;      // first free page in the linked list
    int numPages;       // # of pages in the file
    int extRecordSize;  // record size as seen by users
    int pageSize;       // pageSize as seen by users
    AttrType attrType;

void print()
{
    printf("\n==========RM_FileHdr=============\n");
    printf("firstFree %d \n", firstFree);
    printf("numPages %d \n", numPages);
    printf("extRecordSize %d \n", extRecordSize);
    printf("============RM_FileHdr===========\n\n");
}
};


//
// RM_PageHdr: Header structure for RM page
// 
#define RM_PAGE_LIST_END  -1       // end of list of free pages
#define RM_PAGE_FULLY_USED      -2       // page is fully used with no free slots
struct RM_PageHdr {
    int nextFree; // next free page

    char *freeSlotMap;
    int numTotSlots;
    int numFreeSlots;

RM_PageHdr(int numSlots) : numTotSlots(numSlots), numFreeSlots(numSlots)
{
    freeSlotMap = 
    freeSlotMap = new char[this->mapsize()];
}

~RM_PageHdr()
{
    delete [] freeSlotMap;
}

int size() const
{
    return sizeof(nextFree) + sizeof(numTotSlots) + sizeof(numFreeSlots)
        + bitmap(numTotSlots).NumChars*sizeof(char);
}

int mapsize() const
{
    return this->size() - sizeof(nextFree)
        - sizeof(numTotSlots) - sizeof(numFreeSlots);
}

int to_buf(char *& buf) const
{
    //
    // send PageHdr to 'buf'
    //
    memcpy(buf, &nextFree, sizeof(nextFree));
    memcpy(buf+sizeof(nextFree), &numTotSlots, sizeof(numTotSlots));
    memcpy(buf+sizeof(nextFree)+sizeof(numTotSlots),
        &numFreeSlots, sizeof(numFreeSlots));
    memcpy(buf+sizeof(nextFree)+sizeof(numTotSlots)+sizeof(numFreeSlots),
        freeSlotMap, this->mapsize()*sizeof(char));
    return 0;
}

int from_buf(const char *buf)
{
    //
    // set PageHdr with 'buf'
    //
    memcpy(&nextFree, buf, sizeof(nextFree));
    memcpy(&numTotSlots, buf+sizeof(nextFree), sizeof(numTotSlots));
    memcpy(&numFreeSlots, buf+sizeof(nextFree)+sizeof(numTotSlots), sizeof(numFreeSlots));
    memcpy(freeSlotMap, buf+sizeof(nextFree)+sizeof(numTotSlots)+sizeof(numFreeSlots),
        this->mapsize()*sizeof(char));
    return 0;
}

};


//
// RM_Record: RM Record interface
//
class RM_Record {
private:
    int recordSize;
    char * data;
    RID rid;
public:
    RM_Record ();
    ~RM_Record();

    // Return the data corresponding to the record.  Sets *pData to the
    // record contents.
    RC GetData(char *&pData) const;

    // Return the RID associated with the record
    RC GetRid (RID &rid) const;

    int GetRSize() const {return recordSize;}
    RC Set(const char *pData, int size, RID rid_);
    bool IsNullValue() const;
};


RC CreateRMFile (const char *fileName, int recordSize, int pageSize, AttrType attrType);
RC DestroyRMFile(const char *fileName);

//
// RM_FileHandle: RM File interface
//
class RM_FileHandle {
private:
    RM_FileHdr hdr;
    PF_FileHandle *pfh;
    bool bFileOpen;    // open flag for file
    bool bHdrChanged;  // dirty flag for FileHeader

    RC SetFileHeader(PF_PageHandle ph) const;

    bool IsValidRID(const RID rid) const;

    RC GetSlotPointer(PF_PageHandle ph, SlotNum s, char *& pData) const;
    RC GetNextFreePage(PageNum& pageNum);
    RC GetNextFreeSlot(PF_PageHandle& ph, PageNum& p, SlotNum& s);
public:
    RM_FileHandle ();
    ~RM_FileHandle();
    RC OpenRMFile(const char *fileName);
    RC CloseRMFile();

    RC SetHdr(RM_FileHdr h);
    RC GetPageHeader(PF_PageHandle &ph, RM_PageHdr &pHdr) const;
    RC SetPageHeader(PF_PageHandle &ph, RM_PageHdr &pHdr);
    // Given a RID, return the record
    RC GetRec     (RID rid, RM_Record &rec) const;

    RC InsertRec  (const void *pData, RID &rid);       // Insert a new record
    RC DeleteRec  (const RID &rid);                    // Delete a record
    RC UpdateRec  (const RM_Record &rec);              // Update a record

    RC PinPage(PF_PageHandle &ph, PageNum p);
    RC UnpinPage(PageNum p);
    // Forces a page (along with any contents stored in this class)
    // from the buffer pool to disk.  Default value forces all pages.
    RC ForcePages (PageNum pageNum = ALL_PAGES);

    bool hdrChanged() const;
    PageNum GetNumPages() const;
    SlotNum GetNumSlots() const;
    long long GetNumRecs() const;
    RC WriteAllRids(const char *OutFile) const;
    RC WriteValue(FILE *&fp, RID rid) const;
    RC IsValid() const;
    void print(PageNum p, AttrType type);

    RC ExpandPage(PageNum numPages);
    RC CopyNullFromOthers(RM_FileHandle &rhs);
};

//
// Print-error function
//
void RM_PrintError(RC rc);

#endif
