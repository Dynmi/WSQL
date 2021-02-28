//
// File:        pf.h
// Description: Paged File component interface
// Authors:     Haris Wang  (dynmiw@gmail.com)
//              Hugo Rivero (rivero@cs.stanford.edu)
//              Dallan Quass (quass@cs.stanford.edu)
//              Jason McHugh (mchughj@cs.stanford.edu)
//
// 1997: Default page size is now 4k.
//       Some additional constants used for the statistics.
// 1998: PageNum is now an int instead of short int.
//       Allow chunks from the buffer manager to not be associated with
//       a particular file.  Allows students to use main memory chunks
//       that are associated with (and limited by) the buffer.
// 2005: Added GetLastPage and GetPrevPage for rocking
//
// 2021: Change fixed page size to customed page size
//

#ifndef PF_H
#define PF_H

#include "wsql.h"

//
// PageNum: uniquely identifies a page in a file
//
typedef int PageNum;

/* // Page Size
//
// Each page stores some header information.  The PF_PageHdr is defined
// in pf_internal.h and contains the information that we would store.
// Unfortunately, we cannot use sizeof(PF_PageHdr) here, but it is an
// int and we simply use that.
//
const int PF_PAGE_SIZE = 4096 - sizeof(int); */

//
// PF_PageHandle: PF page interface
//
class PF_PageHandle {
   friend class PF_FileHandle;
public:
   PF_PageHandle  ();                            // Default constructor
   ~PF_PageHandle ();                            // Destructor

   // Copy constructor
   PF_PageHandle  (const PF_PageHandle &pageHandle);
   // Overloaded =
   PF_PageHandle& operator=(const PF_PageHandle &pageHandle);

   RC GetData     (char *&pData) const;           // Set pData to point to
                                                  // the page contents
   RC GetPageNum  (PageNum &pageNum) const;       // Return the page number
private:
   int  pageNum;                                  // page number
   char *pPageData;                               // pointer to page data
};

//
// PF_FileHdr: Header structure for files
//
struct PF_FileHdr {
   int firstFree;     // first free page in the linked list
   int numPages;      // # of pages in the file
   int pageSize;      // exact size of page on disk
};

//
// PF_FileHandle: PF File interface
//
class PF_BufferMgr;

class PF_FileHandle {
   friend class PF_Manager;
public:
   PF_FileHandle  ();                            // Default constructor
   ~PF_FileHandle ();                            // Destructor

   // Copy constructor
   PF_FileHandle  (const PF_FileHandle &fileHandle);

   // Overload =
   PF_FileHandle& operator=(const PF_FileHandle &fileHandle);


   RC OpenFile(const char *fileName);
   RC CloseFile();
   // Three methods that manipulate the buffer manager.  The calls are
   // forwarded to the PF_BufferMgr instance and are called by parse.y
   // when the user types in a system command.
   RC ClearBuffer   ();
   RC PrintBuffer   ();
   RC ResizeBuffer  (int iNewSize);

   // Three Methods for manipulating raw memory buffers.  These memory
   // locations are handled by the buffer manager, but are not
   // associated with a particular file.  These should be used if you
   // want to create structures in memory that are bounded by the size
   // of the buffer pool.
   //
   // Note: If you lose the pointer to the buffer that is passed back
   // from AllocateBlock or do not call DisposeBlock with that pointer
   // then the internal memory will always remain "pinned" in the buffer
   // pool and you will have lost a slot in the buffer pool.

   // Return the size of the block that can be allocated.
   RC GetBlockSize  (int &length) const;

   // Allocate a memory chunk that lives in buffer manager
   RC AllocateBlock (char *&buffer);
   // Dispose of a memory chunk managed by the buffer manager.
   RC DisposeBlock  (char *buffer);


   // Get the first page
   RC GetFirstPage(PF_PageHandle &pageHandle) const;
   // Get the next page after current
   RC GetNextPage (PageNum current, PF_PageHandle &pageHandle) const;
   // Get a specific page
   RC GetThisPage (PageNum pageNum, PF_PageHandle &pageHandle) const;
   // Get the last page
   RC GetLastPage(PF_PageHandle &pageHandle) const;
   // Get the prev page after current
   RC GetPrevPage (PageNum current, PF_PageHandle &pageHandle) const;

   RC AllocatePage(PF_PageHandle &pageHandle);    // Allocate a new page
   RC DisposePage (PageNum pageNum);              // Dispose of a page
   RC MarkDirty   (PageNum pageNum) const;        // Mark page as dirty
   RC UnpinPage   (PageNum pageNum) const;        // Unpin the page

   // Flush pages from buffer pool.  Will write dirty pages to disk.
   RC FlushPages  () const;

   // Force a page or pages to disk (but do not remove from the buffer pool)
   RC ForcePages  (PageNum pageNum=ALL_PAGES) const;

   PageNum GetNumPages() const;
private:
   // IsValidPageNum will return true if page number is valid and false
   // otherwise
   int IsValidPageNum (PageNum pageNum) const;

   PF_BufferMgr *pBufferMgr;
   PF_FileHdr hdr;                                // file header
   int bFileOpen;                                 // file open flag
   int bHdrChanged;                               // dirty flag for file hdr
   int unixfd;                                    // OS file descriptor
};


RC PF_CreateFile    (const char *fileName, int _pageSize);       // Create a new file
RC PF_DestroyFile   (const char *fileName);       // Delete a file





//
// Constants and defines
//
const int PF_BUFFER_SIZE = 40;     // Number of pages in the buffer
const int PF_HASH_TBL_SIZE = 20;   // Size of hash table

#define CREATION_MASK      0600    // r/w privileges to owner only
#define PF_PAGE_LIST_END  -1       // end of list of free pages
#define PF_PAGE_USED      -2       // page is being used

// L_SET is used to indicate the "whence" argument of the lseek call
// defined in "/usr/include/unistd.h".  A value of 0 indicates to
// move to the absolute location specified.
#ifndef L_SET
#define L_SET              0
#endif

//
// PF_PageHdr: Header structure for pages
//
struct PF_PageHdr {
    int nextFree;       // nextFree can be any of these values:
                        //  - the number of the next free page
                        //  - PF_PAGE_LIST_END if this is last free page
                        //  - PF_PAGE_USED if the page is not free
};

/* // Justify the file header to the length of one page
const int PF_FILE_HDR_SIZE = PF_PAGE_SIZE + sizeof(PF_PageHdr); */



//
// Print-error function and PF return code defines
//
void PF_PrintError(RC rc);

#define PF_PAGEPINNED      (START_PF_WARN + 0) // page pinned in buffer
#define PF_PAGENOTINBUF    (START_PF_WARN + 1) // page isn't pinned in buffer
#define PF_INVALIDPAGE     (START_PF_WARN + 2) // invalid page number
#define PF_FILEOPEN        (START_PF_WARN + 3) // file is open
#define PF_CLOSEDFILE      (START_PF_WARN + 4) // file is closed
#define PF_PAGEFREE        (START_PF_WARN + 5) // page already free
#define PF_PAGEUNPINNED    (START_PF_WARN + 6) // page already unpinned
#define PF_EOF             (START_PF_WARN + 7) // end of file
#define PF_TOOSMALL        (START_PF_WARN + 8) // Resize buffer too small
#define PF_LASTWARN        PF_TOOSMALL

#define PF_NOMEM           (START_PF_ERR - 0)  // no memory
#define PF_NOBUF           (START_PF_ERR - 1)  // no buffer space
#define PF_INCOMPLETEREAD  (START_PF_ERR - 2)  // incomplete read from file
#define PF_INCOMPLETEWRITE (START_PF_ERR - 3)  // incomplete write to file
#define PF_HDRREAD         (START_PF_ERR - 4)  // incomplete read of header
#define PF_HDRWRITE        (START_PF_ERR - 5)  // incomplete write to header

// Internal errors
#define PF_PAGEINBUF       (START_PF_ERR - 6) // new page already in buffer
#define PF_HASHNOTFOUND    (START_PF_ERR - 7) // hash table entry not found
#define PF_HASHPAGEEXIST   (START_PF_ERR - 8) // page already in hash table
#define PF_INVALIDNAME     (START_PF_ERR - 9) // invalid PC file name

// Error in UNIX system call or library routine
#define PF_UNIX            (START_PF_ERR - 10) // Unix error
#define PF_LASTERROR       PF_UNIX

#endif
