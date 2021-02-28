//
// File:        rm_record.cc
//
// Description: rm_record class implementation
//
// Author:     Haris Wang (dynmiw@gmail.com)
//
// 
#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include "rm.h"


RM_Record::RM_Record():recordSize(-1), data(NULL), rid(-1,-1){}


RM_Record::~RM_Record()
{
    if (data != NULL)
        delete [] data;
}


// Return the data corresponding to the record.  Sets *pData to the
// record contents.
RC RM_Record::GetData(char *&pData) const
{
    if (data==NULL | recordSize==-1)
    {
        return (START_RM_ERR-2);   
    }

    pData = data;
    return 0;
}


// Return the RID associated with the record
RC RM_Record::GetRid (RID &rid) const
{
    if (data==NULL | recordSize==-1)
    {
        return (START_RM_ERR-2);   
    }

    rid = this->rid;
    return 0;
}


//Sets data in the record for a fixed record-size
RC RM_Record::Set(const char *pData, int size, RID rid_)
{
    recordSize = size;
    this->rid = rid_;
    if (data == NULL)
        data = new char[recordSize];
    memcpy(data, pData, size);
    return 0;
}


//
// check whether the data is NULL
//
bool RM_Record::IsNullValue() const
{
    if (data == NULL)
        return 0;
    int p = 0;
    while (p < recordSize)
    {
        if (data[p] != 0x00)
            break;
        p++;
    }
    if (p == recordSize)
    {
        return 1;
    }else {
        return 0;
    }
}
