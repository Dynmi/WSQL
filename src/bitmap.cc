//
// File:        bitmap.cc
//
// Description: bitmap class implementation
//
// Author:     Haris Wang (dynmiw@gmail.com)
// 

#include <cstdio>
#include <cstring>
#include <cassert>
#include <climits>
#include <math.h>
#include <unistd.h>
#include <iostream>
#include "rm.h"


//
// allocate memory space for buffer
//
bitmap::bitmap(int numBits, char *buf)
{
    size = numBits;
    NumChars = ceil(1.0 * size / 8);
    buffer = new char[NumChars];

    if(buf==NULL)
    {
        memset((void*)buffer, 0 , NumChars);
    }
    else
    {
        memcpy(buffer, buf, NumChars);
    }
    
}


//
// recycle memory space for buffer
//
bitmap::~bitmap()
{
    delete [] buffer;
}


//
// copy the data in 'buffer' to 'buf'
//
RC bitmap::to_char_buf(char * buf, int len) const
{
    assert( buf!=NULL && len==NumChars );

    memcpy((void*)buf, buffer, len);
    return 0;
}


//
// set the bit at 'pos' to 'sign'
//
void bitmap::set(unsigned int pos, bool sign)
{
    int byte, offset;
    
    if (pos != UINT_MAX)
    {
        assert(pos < size);
        // set given bit to 1/0
        byte = pos / 8;
        offset = pos % 8;  
        if(sign)
        {
            buffer[byte] |= (1 << offset); 
        }
        else
        {
            buffer[byte] &= ~(1 << offset);
        }
    }else {
        // set all bits to 1/0
        for(pos = 0; pos < size; pos++)
        {
            byte = pos / 8;
            offset = pos % 8;  
            if(sign)
            {
                buffer[byte] |= (1 << offset); 
            }
            else
            {
                buffer[byte] &= ~(1 << offset);
            }
        }
    }
    
}


RC bitmap::getSize() const
{
    return size;
}

bool bitmap::test(unsigned int bitNumber) const
{
    assert(bitNumber <= size - 1);
    int byte = bitNumber/8;
    int offset = bitNumber%8;

    return buffer[byte] & (1 << offset);
}