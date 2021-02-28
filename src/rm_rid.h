//
// File:        rm_rid.h
//
// Description: The Record Id interface
//
// Author:     Haris Wang (dynmiw@gmail.com)
//
// 
#ifndef RM_RID_H
#define RM_RID_H

// We separate the interface of RID from the rest of RM because some
// components will require the use of RID but not the rest of RM.

#include "wsql.h"

//
// PageNum: uniquely identifies a page in a file
//
typedef int PageNum;

//
// SlotNum: uniquely identifies a record in a page
//
typedef int SlotNum;

#define NULL_PAGE -1
#define NULL_SLOT -1
//
// RID: Record id interface
//
struct RID {
public:
    PageNum page;
    SlotNum slot;
    RID():page(NULL_PAGE),slot(NULL_SLOT){}          // Default constructor
    RID(PageNum p, SlotNum s):page(p),slot(s){}  
    ~RID(){};                                        // Destructor

    bool operator==(const RID & that) const
    {
        return (that.page == page && that.slot == slot);
    }
    void print()
    {
        printf("(%d,%d)",page,slot);
    }
};

#endif
