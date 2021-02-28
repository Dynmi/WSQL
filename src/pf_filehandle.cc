//
// File:        pf_filehandle.cc
// Description: PF_FileHandle class implementation
// Authors:     Haris Wang  (dynmiw@gmail.com)   
//              Hugo Rivero (rivero@cs.stanford.edu)
//              Dallan Quass (quass@cs.stanford.edu)
//

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "pf.h"
#include "pf_buffermgr.h"


//
// CreateFile
//
// Desc: Create a new PF file named fileName. Initialize PF_FileHeader.
// The size of page space visiable to high level user is defined 
// by '_pageSize'. The real page size on disk should be 
// '_pageSize'+sizeof(PF_PageHdr) .
//
RC PF_CreateFile    (const char *fileName, int _pageSize)
{
   int fd;		// unix file descriptor
   int numBytes;		// return code form write syscall

   // Create file for exclusive use
   if ((fd = open(fileName,
#ifdef PC
         O_BINARY |
#endif
         O_CREAT | O_EXCL | O_WRONLY,
         CREATION_MASK)) < 0)
      return (PF_UNIX);

   int PAGESIZE = _pageSize + sizeof(PF_PageHdr);
   // Initialize the file header: must reserve FileHdrSize bytes in memory
   // though the actual size of FileHdr is smaller
   char hdrBuf[PAGESIZE];

   // So that Purify doesn't complain
   memset(hdrBuf, 0, PAGESIZE);

   PF_FileHdr *hdr = (PF_FileHdr*)hdrBuf;
   hdr->firstFree = PF_PAGE_LIST_END;
   hdr->numPages = 0;
   hdr->pageSize = PAGESIZE;

   // Write header to file
   if((numBytes = write(fd, hdrBuf, PAGESIZE)) != PAGESIZE) 
   {
      // Error while writing: close and remove file
      close(fd);
      unlink(fileName);

      // Return an error
      if(numBytes < 0)
         return (PF_UNIX);
      else
         return (PF_HDRWRITE);
   }

   // Close file
   if(close(fd) < 0)
      return (PF_UNIX);

   // Return ok
   return 0;
}


//
// DestroyFile
//
// Desc: Delete a PF file named fileName (fileName must exist and not be open)
// In:   fileName - name of file to delete
// Ret:  PF return code
//
RC PF_DestroyFile   (const char *fileName)
{
   // Remove the file
   if (unlink(fileName) < 0)
      return (PF_UNIX);

   // Return ok
   return (0);
}


//
// PF_FileHandle
//
// Desc: Default constructor for a file handle object
//       A File object provides access to an open file.
//       It is used to allocate, dispose and fetch pages.
//       It is constructed here but must be passed to PF_Manager::OpenFile() in
//       order to be used to access the pages of a file.
//       It should be passed to PF_Manager::CloseFile() to close the file.
//       A file handle object contains a pointer to the file data stored
//       in the file table managed by PF_Manager.  It passes the file's unix
//       file descriptor to the buffer manager to access pages of the file.
//
PF_FileHandle::PF_FileHandle()
{
   // Initialize local variables
   bFileOpen = false;
   pBufferMgr = NULL;
}

//
// ~PF_FileHandle
//
// Desc: Destroy the file handle object
//       If the file handle object refers to an open file, the file will
//       NOT be closed.
//
PF_FileHandle::~PF_FileHandle()
{
   // Don't need to do anything
}

//
// PF_FileHandle
//
// Desc: copy constructor
// In:   fileHandle - file handle object from which to construct this object
//
PF_FileHandle::PF_FileHandle(const PF_FileHandle &fileHandle)
{
   // Just copy the data members since there is no memory allocation involved
   this->pBufferMgr  = fileHandle.pBufferMgr;
   this->hdr         = fileHandle.hdr;
   this->bFileOpen   = fileHandle.bFileOpen;
   this->bHdrChanged = fileHandle.bHdrChanged;
   this->unixfd      = fileHandle.unixfd;
}

//
// operator=
//
// Desc: overload = operator
//       If this file handle object refers to an open file, the file will
//       NOT be closed.
// In:   fileHandle - file handle object to set this object equal to
// Ret:  reference to *this
//
PF_FileHandle& PF_FileHandle::operator= (const PF_FileHandle &fileHandle)
{
   // Test for self-assignment
   if (this != &fileHandle) {

      // Just copy the members since there is no memory allocation involved
      this->pBufferMgr  = fileHandle.pBufferMgr;
      this->hdr         = fileHandle.hdr;
      this->bFileOpen   = fileHandle.bFileOpen;
      this->bHdrChanged = fileHandle.bHdrChanged;
      this->unixfd      = fileHandle.unixfd;
   }

   // Return a reference to this
   return (*this);
}


//
// OpenFile
//
// Desc: Open the paged file whose name is "fileName".  It is possible to open
//       a file more than once, however, it will be treated as 2 separate files
//       (different file descriptors; different buffers).  Thus, opening a file
//       more than once for writing may corrupt the file, and can, in certain
//       circumstances, crash the PF layer. Note that even if only one instance
//       of a file is for writing, problems may occur because some writes may
//       not be seen by a reader of another instance of the file.
// In:   fileName - name of file to open
// Ret:  PF_FILEOPEN or other PF return code
//
RC PF_FileHandle::OpenFile(const char *fileName)
{
   RC rc = 0;

   // Ensure file is not already open
   if (bFileOpen | pBufferMgr != NULL)
      return (PF_FILEOPEN);

   // Open the file
   if ((unixfd = open(fileName,
#ifdef PC
         O_BINARY |
#endif
         O_RDWR)) < 0)
      return (PF_UNIX);

   // Read the file header
   {
      int numBytes = read(unixfd, (char *)&hdr, sizeof(PF_FileHdr));
      if (numBytes != sizeof(PF_FileHdr)) {
         rc = (numBytes < 0) ? PF_UNIX : PF_HDRREAD;
         goto err;
      }
   }

   // Set file header to be not changed
   bHdrChanged = false;

   pBufferMgr = new PF_BufferMgr(PF_BUFFER_SIZE, hdr.pageSize);
   bFileOpen = true;

   // Return ok
   return 0;

err:
   // Close file
   close(unixfd);
   bFileOpen = false;

   return rc;
}


//
// CloseFile
//
// Desc: Close file associated with fileHandle
//       The file should have been opened with OpenFile().
//       Also, flush all pages for the file from the page buffer
//       It is an error to close a file with pages still fixed in the buffer.
// Ret:  PF return code
//
RC PF_FileHandle::CloseFile()
{
   RC rc = 0;

   // Ensure fileHandle refers to open file
   if (!bFileOpen)
      return (PF_CLOSEDFILE);

   // Flush all buffers for this file and write out the header
   if ((rc = this->FlushPages()))
      return (rc);

   // Close the file
   if (close(unixfd) < 0)
      return (PF_UNIX);
   bFileOpen = false;

   // Reset the buffer manager pointer in the file handle
   delete pBufferMgr;
   pBufferMgr = NULL;

   // Return ok
   return rc;
}


//
// ClearBuffer
//
// Desc: Remove all entries from the buffer manager.
//       This routine will be called via the system command and is only
//       really useful if the user wants to run some performance
//       comparison starting with an clean buffer.
// In:   Nothing
// Out:  Nothing
// Ret:  Returns the result of PF_BufferMgr::ClearBuffer
//       It is a code: 0 for success, something else for a PF error.
//
RC PF_FileHandle::ClearBuffer()
{
   return pBufferMgr->ClearBuffer();
}

//
// PrintBuffer
//
// Desc: Display all of the pages within the buffer.
//       This routine will be called via the system command.
// In:   Nothing
// Out:  Nothing
// Ret:  Returns the result of PF_BufferMgr::PrintBuffer
//       It is a code: 0 for success, something else for a PF error.
//
RC PF_FileHandle::PrintBuffer()
{
   return pBufferMgr->PrintBuffer();
}

//
// ResizeBuffer
//
// Desc: Resizes the buffer manager to the size passed in.
//       This routine will be called via the system command.
// In:   The new buffer size
// Out:  Nothing
// Ret:  Returns the result of PF_BufferMgr::ResizeBuffer
//       It is a code: 0 for success, PF_TOOSMALL when iNewSize
//       would be too small.
//
RC PF_FileHandle::ResizeBuffer(int iNewSize)
{
   return pBufferMgr->ResizeBuffer(iNewSize);
}

//------------------------------------------------------------------------------
// Three Methods for manipulating raw memory buffers.  These memory
// locations are handled by the buffer manager, but are not
// associated with a particular file.  These should be used if you
// want memory that is bounded by the size of the buffer pool.
//
// The PF_Manager just passes the calls down to the Buffer manager.
//------------------------------------------------------------------------------

RC PF_FileHandle::GetBlockSize(int &length) const
{
   return pBufferMgr->GetBlockSize(length);
}

RC PF_FileHandle::AllocateBlock(char *&buffer)
{
   return pBufferMgr->AllocateBlock(buffer);
}

RC PF_FileHandle::DisposeBlock(char *buffer)
{
   return pBufferMgr->DisposeBlock(buffer);
}


//
// GetFirstPage
//
// Desc: Get the first page in a file
//       The file handle must refer to an open file
// Out:  pageHandle - becomes a handle to the first page of the file
//       The referenced page is pinned in the buffer pool.
// Ret:  PF return code
//
RC PF_FileHandle::GetFirstPage(PF_PageHandle &pageHandle) const
{
   return (GetNextPage((PageNum)-1, pageHandle));
}

//
// GetLastPage
//
// Desc: Get the last page in a file
//       The file handle must refer to an open file
// Out:  pageHandle - becomes a handle to the last page of the file
//       The referenced page is pinned in the buffer pool.
// Ret:  PF return code
//
RC PF_FileHandle::GetLastPage(PF_PageHandle &pageHandle) const
{
   return (GetPrevPage((PageNum)hdr.numPages, pageHandle));
}

//
// GetNextPage
//
// Desc: Get the next (valid) page after current
//       The file handle must refer to an open file
// In:   current - get the next valid page after this page number
//       current can refer to a page that has been disposed
// Out:  pageHandle - becomes a handle to the next page of the file
//       The referenced page is pinned in the buffer pool.
// Ret:  PF_EOF, or another PF return code
//
RC PF_FileHandle::GetNextPage(PageNum current, PF_PageHandle &pageHandle) const
{
   int rc;               // return code

   // File must be open
   if (!bFileOpen)
      return (PF_CLOSEDFILE);

   // Validate page number (note that -1 is acceptable here)
   if (current != -1 &&  !IsValidPageNum(current))
      return (PF_INVALIDPAGE);

   // Scan the file until a valid used page is found
   for (current++; current < hdr.numPages; current++) {

      // If this is a valid (used) page, we're done
      if (!(rc = GetThisPage(current, pageHandle)))
         return (0);

      // If unexpected error, return it
      if (rc != PF_INVALIDPAGE)
         return (rc);
   }

   // No valid (used) page found
   return (PF_EOF);
}

//
// GetPrevPage
//
// Desc: Get the prev (valid) page after current
//       The file handle must refer to an open file
// In:   current - get the prev valid page before this page number
//       current can refer to a page that has been disposed
// Out:  pageHandle - becomes a handle to the prev page of the file
//       The referenced page is pinned in the buffer pool.
// Ret:  PF_EOF, or another PF return code
//
RC PF_FileHandle::GetPrevPage(PageNum current, PF_PageHandle &pageHandle) const
{
   int rc;               // return code

   // File must be open
   if (!bFileOpen)
      return (PF_CLOSEDFILE);

   // Validate page number (note that hdr.numPages is acceptable here)
   if (current != hdr.numPages &&  !IsValidPageNum(current))
      return (PF_INVALIDPAGE);

   // Scan the file until a valid used page is found
   for (current--; current >= 0; current--) {

      // If this is a valid (used) page, we're done
      if (!(rc = GetThisPage(current, pageHandle)))
         return (0);

      // If unexpected error, return it
      if (rc != PF_INVALIDPAGE)
         return (rc);
   }

   // No valid (used) page found
   return (PF_EOF);
}

//
// GetThisPage
//
// Desc: Get a specific page in a file
//       The file handle must refer to an open file
// In:   pageNum - the number of the page to get
// Out:  pageHandle - becomes a handle to the this page of the file
//                    this function modifies local var's in pageHandle
//       The referenced page is pinned in the buffer pool.
// Ret:  PF return code
//
RC PF_FileHandle::GetThisPage(PageNum pageNum, PF_PageHandle &pageHandle) const
{
   int  rc;               // return code
   char *pPageBuf;        // address of page in buffer pool

   // File must be open
   if (!bFileOpen)
      return (PF_CLOSEDFILE);

   // Validate page number
   if (!IsValidPageNum(pageNum))
      return (PF_INVALIDPAGE);

   // Get this page from the buffer manager
   if ((rc = pBufferMgr->GetPage(unixfd, pageNum, &pPageBuf)))
      return (rc);

   // If the page is valid, then set pageHandle to this page and return ok
   if (((PF_PageHdr*)pPageBuf)->nextFree == PF_PAGE_USED) {
      // Set the pageHandle local variables
      pageHandle.pageNum = pageNum;
      pageHandle.pPageData = pPageBuf + sizeof(PF_PageHdr);

      // Return ok
      return (0);
   }

   // If the page is *not* a valid one, then unpin the page
   if ((rc = UnpinPage(pageNum)))
      return (rc);

   return (PF_INVALIDPAGE);
}

//
// AllocatePage
//
// Desc: Allocate a new page in the file (may get a page which was
//       previously disposed)
//       The file handle must refer to an open file
// Out:  pageHandle - becomes a handle to the newly-allocated page
//                    this function modifies local var's in pageHandle
// Ret:  PF return code
//
RC PF_FileHandle::AllocatePage(PF_PageHandle &pageHandle)
{
   int     rc;               // return code
   int     pageNum;          // new-page number
   char    *pPageBuf;        // address of page in buffer pool

   // File must be open
   if (!bFileOpen)
      return (PF_CLOSEDFILE);

   // If the free list isn't empty...
   if (hdr.firstFree != PF_PAGE_LIST_END) {
      pageNum = hdr.firstFree;

      // Get the first free page into the buffer
      if ((rc = pBufferMgr->GetPage(unixfd,
            pageNum,
            &pPageBuf)))
         return (rc);

      // Set the first free page to the next page on the free list
      hdr.firstFree = ((PF_PageHdr*)pPageBuf)->nextFree;
   }
   else {

      // The free list is empty...
      pageNum = hdr.numPages;

      // Allocate a new page in the file
      if ((rc = pBufferMgr->AllocatePage(unixfd,
            pageNum,
            &pPageBuf)))
         return (rc);

      // Increment the number of pages for this file
      hdr.numPages++;
   }

   // Mark the header as changed
   bHdrChanged = true;

   // Mark this page as used
   ((PF_PageHdr *)pPageBuf)->nextFree = PF_PAGE_USED;

   // Zero out the page data
   memset(pPageBuf + sizeof(PF_PageHdr), 0, hdr.pageSize - sizeof(PF_PageHdr));

   // Mark the page dirty because we changed the next pointer
   if ((rc = MarkDirty(pageNum)))
      return (rc);

   // Set the pageHandle local variables
   pageHandle.pageNum = pageNum;
   pageHandle.pPageData = pPageBuf + sizeof(PF_PageHdr);

   // Return ok
   return (0);
}

//
// DisposePage
//
// Desc: Dispose of a page
//       The file handle must refer to an open file
//       PF_PageHandle objects referring to this page should not be used
//       after making this call.
// In:   pageNum - number of page to dispose
// Ret:  PF return code
//
RC PF_FileHandle::DisposePage(PageNum pageNum)
{
   int     rc;               // return code
   char    *pPageBuf;        // address of page in buffer pool

   // File must be open
   if (!bFileOpen)
      return (PF_CLOSEDFILE);

   // Validate page number
   if (!IsValidPageNum(pageNum))
      return (PF_INVALIDPAGE);

   // Get the page (but don't re-pin it if it's already pinned)
   if ((rc = pBufferMgr->GetPage(unixfd,
         pageNum,
         &pPageBuf,
         false)))
      return (rc);

   // Page must be valid (used)
   if (((PF_PageHdr *)pPageBuf)->nextFree != PF_PAGE_USED) {

      // Unpin the page
      if ((rc = UnpinPage(pageNum)))
         return (rc);

      // Return page already free
      return (PF_PAGEFREE);
   }

   // Put this page onto the free list
   ((PF_PageHdr *)pPageBuf)->nextFree = hdr.firstFree;
   hdr.firstFree = pageNum;
   bHdrChanged = true;

   // Mark the page dirty because we changed the next pointer
   if ((rc = MarkDirty(pageNum)))
      return (rc);

   // Unpin the page
   if ((rc = UnpinPage(pageNum)))
      return (rc);

   // Return ok
   return (0);
}

//
// MarkDirty
//
// Desc: Mark a page as being dirty
//       The page will then be written back to disk when it is removed from
//       the page buffer
//       The file handle must refer to an open file
// In:   pageNum - number of page to mark dirty
// Ret:  PF return code
//
RC PF_FileHandle::MarkDirty(PageNum pageNum) const
{
   // File must be open
   if (!bFileOpen)
      return (PF_CLOSEDFILE);

   // Validate page number
   if (!IsValidPageNum(pageNum))
      return (PF_INVALIDPAGE);

   // Tell the buffer manager to mark the page dirty
   return (pBufferMgr->MarkDirty(unixfd, pageNum));
}

//
// UnpinPage
//
// Desc: Unpin a page from the buffer manager.
//       The page is then free to be written back to disk when necessary.
//       PF_PageHandle objects referring to this page should not be used
//       after making this call.
//       The file handle must refer to an open file.
// In:   pageNum - number of the page to unpin
// Ret:  PF return code
//
RC PF_FileHandle::UnpinPage(PageNum pageNum) const
{
   // File must be open
   if (!bFileOpen)
      return (PF_CLOSEDFILE);

   // Validate page number
   if (!IsValidPageNum(pageNum))
      return (PF_INVALIDPAGE);

   // Tell the buffer manager to unpin the page
   return (pBufferMgr->UnpinPage(unixfd, pageNum));
}

//
// FlushPages
//
// Desc: Flush all dirty unpinned pages from the buffer manager for this file
// In:   Nothing
// Ret:  PF_PAGEFIXED warning from buffer manager if pages are pinned or
//       other PF error
//
RC PF_FileHandle::FlushPages() const
{
   // File must be open
   if (!bFileOpen)
      return (PF_CLOSEDFILE);

   // If the file header has changed, write it back to the file
   if (bHdrChanged) {

      // First seek to the appropriate place
      if (lseek(unixfd, 0, L_SET) < 0)
         return (PF_UNIX);

      // Write header
      int numBytes = write(unixfd,
            (char *)&hdr,
            sizeof(PF_FileHdr));
      if (numBytes < 0)
         return (PF_UNIX);
      if (numBytes != sizeof(PF_FileHdr))
         return (PF_HDRWRITE);

      // This function is declared const, but we need to change the
      // bHdrChanged variable.  Cast away the constness
      PF_FileHandle *dummy = (PF_FileHandle *)this;
      dummy->bHdrChanged = false;
   }

   // Tell Buffer Manager to flush pages
   return (pBufferMgr->FlushPages(unixfd));
}

//
// ForcePages
//
// Desc: If a page is dirty then force the page from the buffer pool
//       onto disk.  The page will not be forced out of the buffer pool.
// In:   The page number, a default value of ALL_PAGES will be used if
//       the client doesn't provide a value.  This will force all pages.
// Ret:  Standard PF errors
//
//
RC PF_FileHandle::ForcePages(PageNum pageNum) const
{
   // File must be open
   if (!bFileOpen)
      return (PF_CLOSEDFILE);

   // If the file header has changed, write it back to the file
   if (bHdrChanged) {

      // First seek to the appropriate place
      if (lseek(unixfd, 0, L_SET) < 0)
         return (PF_UNIX);

      // Write header
      int numBytes = write(unixfd,
            (char *)&hdr,
            sizeof(PF_FileHdr));
      if (numBytes < 0)
         return (PF_UNIX);
      if (numBytes != sizeof(PF_FileHdr))
         return (PF_HDRWRITE);

      // This function is declared const, but we need to change the
      // bHdrChanged variable.  Cast away the constness
      PF_FileHandle *dummy = (PF_FileHandle *)this;
      dummy->bHdrChanged = false;
   }

   // Tell Buffer Manager to Force the page
   return (pBufferMgr->ForcePages(unixfd, pageNum));
}


//
//
//
PageNum PF_FileHandle::GetNumPages() const
{
   return hdr.numPages;
}


//
// IsValidPageNum
//
// Desc: Internal.  Return true if pageNum is a valid page number
//       in the file, false otherwise
// In:   pageNum - page number to test
// Ret:  true or false
//
int PF_FileHandle::IsValidPageNum(PageNum pageNum) const
{
   return (bFileOpen &&
         pageNum >= 0 &&
         pageNum < hdr.numPages);
}

