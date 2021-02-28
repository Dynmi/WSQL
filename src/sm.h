//
// File:        sm.h
//
// Description: System Manager component interface
//
// Author:     Haris Wang (dynmiw@gmail.com)
//
// 
#ifndef SM_H
#define SM_H

#include <bits/stdc++.h>

#include "wsql.h"
#include "ix.h"
#include "rm.h"

using namespace std;

struct attrInfo {
    string name;
    AttrType  type;
    int  length;
    
    attrInfo():name(""),type(NULL_TYPE),length(-1){}

attrInfo(string _name, AttrType _type, int _length)
{
    name = _name;
    type = _type;
    length = _length;
}

string GetAttrType()
{
    switch (type)
    {
    case STRING:{
        return "STRING";
    }break;
    case INT:{
        return "INT";
    }break;
    case FLOAT:{
        return "FLOAT";
    }break;
    default:

        break;
    }
}

void print()
{
    printf("\n==========attrInfo=============\n");
    printf("name %s \n", name.c_str());
    auto ctype = GetAttrType();
    printf("attrType %s \n", ctype.c_str());
    printf("length %d \n", length);
    printf("============attrInfo===========\n\n");
}
};


class SM_TableHandle {
private:
    string dbPath;

public:

    SM_TableHandle(string &database_path);
    ~SM_TableHandle();

    RC CreateTable(string &tableName, vector<attrInfo> *attrList = NULL);
    RC DropTable(string &tableName);
    RC RenameTable(string &oldName, string &newName);

    RC AddColumn(string &tableName, attrInfo &colinfo);
    RC DropColumn(string &tableName, string &colName);
    RC RenameColumn(string &tableName, string &oldName, string &newName);

    RC InsertEntry(string &tableName, map<string,string> &entry, RID &_rid);
    RC DeleteEntry(string &tableName, vector<RID> &rids);
    RC UpdateEntry(string &tableName, RID rid, map<string,string> &entry);
    RC SelectEntry(string &tableName, string &retFile, string &column, CompOp &op, void *&cmpKey);
    RC SelectEntry(string &tableName, string &retFile, string &column, CompOp &op, string &value);
    RC SelectEntry_from_file(string &tableName, string &retFile, string &column, CompOp &op, void *&cmpKey);

    RC DetailTable(string &tableName);
    RC SelectTable(string &tableName, string &retFile);
    RC WriteValue(string &tableName, vector<string> &colList, string &outFile, string &RidFile);

    void GetScmFile(string &retFile, string &tableName) const;
    void GetRMFile(string &retFile, string &tableName, string &columnName) const;
    void GetIXFile(string &retFile, string &tableName, string &columnName) const;
};

/* class SM_DatabaseHandle {
private:

public:

}; */

#endif