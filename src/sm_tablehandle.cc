//
// File:        sm_tablehandle.cc
//
// Description: SM_TableHandle class implementation
//
// Author:     Haris Wang (dynmiw@gmail.com)
//
// 
#include "sm.h"
#include <unistd.h>
#include <dirent.h>
#include <bits/stdc++.h>
#define SLOTS_PER_PAGE 256
#define MAX_COL_NAME_LENGTH 64

void list_files(const char *PATH, vector<string> &list)
{
    struct dirent *ptr;
    DIR *dir;
    dir=opendir(PATH);
    while((ptr=readdir(dir))!=NULL)
    {
        if(ptr->d_name[0] == '.')
            continue;
        list.push_back(ptr->d_name);
    }
}


void read_scm(const char *scmPath, vector<attrInfo> &attrList)
{
    int attrNum;
    FILE *fp = fopen(scmPath, "r");
    fscanf(fp, "%d", &attrNum);
    attrInfo tmp;
    char *cname = new char[256];
    for (int i = 0; i < attrNum; i++)
    {
        fscanf(fp, "%s %d %d", 
                 cname, 
                 &(tmp.type), 
                 &(tmp.length));
        tmp.name = cname;
        attrList.push_back(tmp);
    }
    delete cname;
    fclose(fp);
}


void write_scm(const char *scmPath, vector<attrInfo> &attrList)
{
    FILE *fp = fopen(scmPath, "w");
    fprintf(fp, "%d\n", attrList.size());
    for (int i = 0; i < attrList.size(); i++)
    {
        fprintf(fp, "%s %d %d\n", 
                    attrList[i].name.c_str(), 
                    attrList[i].type, 
                    attrList[i].length);
    }
    fclose(fp);
}


RC get_attr_from_scm(const char *scmPath, string &name, attrInfo &info)
{
    FILE *fp = fopen(scmPath, "r");
    int num;
    fscanf(fp, "%d", &num);
    int p = 0;
    char *cname = new char[256];
    while (p < num)
    {
        fscanf(fp, "%s %d %d", 
                 cname, 
                 &(info.type), 
                 &(info.length));
        if (cname == name)
            break;
        p++;
    }
    delete cname;
    fclose(fp);

    if (p == num)
    {
        return -1;
    }else {
        return 0;
    }
}


SM_TableHandle::SM_TableHandle(string &database_path)
{
    this->dbPath = database_path;
}


SM_TableHandle::~SM_TableHandle()
{
    
}


// write path of .scm file to 'retName' 
void SM_TableHandle::GetScmFile(string &retName, string &tableName) const
{
    retName = dbPath + tableName + SCHEMA_SUFFIX;
}

// write path of .data file to 'retName' 
void SM_TableHandle::GetRMFile(string &retName, string &tableName, string &columnName) const
{
    retName = dbPath + tableName + "." + columnName + RM_SUFFIX;
}

// write path of .index file to 'retName' 
void SM_TableHandle::GetIXFile(string &retName, string &tableName, string &columnName) const
{
    retName = dbPath + tableName + "." + columnName + IX_SUFFIX;
}


// 
// create table as instructed. 
// <tableName>.scm file will be created. For each attribution in attrList, 
// <tableName>.<colName>.data & <tableName>.<colName>.index file will be
// created.
// 
// return 1 if table already exists
// return 0 if success
//
RC SM_TableHandle::CreateTable(string &tableName, vector<attrInfo> *attrList)
{
    RC rc = 0;
    string filename;
    GetScmFile(filename, tableName);
    int fexist = access(filename.c_str(), 0);
    if (fexist == 0) return 1;
    if (attrList == NULL)
    {
        FILE *fp = fopen(filename.c_str(), "w");
        fprintf(fp, "0\n");
        fclose(fp);
    }else {
        write_scm(filename.c_str(), *attrList);
        for (auto iter = attrList->begin(); iter != attrList->end(); iter++)
        {
            GetRMFile(filename, tableName, iter->name);
            cout << filename << endl;
            rc = CreateRMFile(filename.c_str(), iter->length, SLOTS_PER_PAGE * iter->length, iter->type);
            if (rc != 0) return rc;

            GetIXFile(filename, tableName, iter->name);
            cout << filename << endl;

            rc = CreateIXFile(filename.c_str(), iter->type, iter->length);
            if (rc != 0) return rc;
        }
    }

    return rc;
}


// 
// drop table as instructed.
// <tableName>.scm file & <tableName>.<colName>.data 
// & <tableName>.<colName>.index files will be destroyed.
// return 0 if success
//
RC SM_TableHandle::DropTable(string &tableName)
{
    RC rc = 0;
    string filename;

    vector<attrInfo> attrList;
    GetScmFile(filename, tableName);
    read_scm(filename.c_str(), attrList);
    rc = remove(filename.c_str());
    if (rc != 0) return 1;

    for (auto iter = attrList.begin(); iter != attrList.end(); iter++)
    {
        GetRMFile(filename, tableName, iter->name);
        rc = DestroyRMFile(filename.c_str());
        if (rc != 0) return rc;

        GetIXFile(filename, tableName, iter->name);
        rc = DestroyIXFile(filename.c_str());
        if (rc != 0) return rc;
    }

    return rc;
}


//
// Delete all records in this table
//
RC SM_TableHandle::ClearTable(string &tableName)
{
    RC rc = 0;
    string filename;
    vector<attrInfo> attrList;
    GetScmFile(filename, tableName);
    read_scm(filename.c_str(), attrList);

    for (auto iter = attrList.begin(); iter != attrList.end(); iter++)
    {
        GetRMFile(filename, tableName, iter->name);
        DestroyRMFile(filename.c_str());
        rc = CreateRMFile(filename.c_str(), iter->length, SLOTS_PER_PAGE * iter->length, iter->type);
        if (rc != 0) return rc;

        GetIXFile(filename, tableName, iter->name);
        DestroyIXFile(filename.c_str());
        rc = CreateIXFile(filename.c_str(), iter->type, iter->length);
        if (rc != 0) return rc;
    }

    return rc;
}

RC SM_TableHandle::RenameTable(string &oldName, string &newName)
{
    RC rc = 0;
    string oldFile, newFile, suffix;
    string cmpStr = oldName + ".";
    int len_cmpStr = cmpStr.size();
    vector<string> files;
    list_files(dbPath.c_str(), files);
    for (int i = 0; i < files.size(); i++)
    {
        if (files[i].size() <= len_cmpStr 
            || strncmp(files[i].c_str(), cmpStr.c_str(), len_cmpStr) != 0) 
        {
            continue;
        }

        suffix = files[i].substr(len_cmpStr-1);
        oldFile = dbPath + oldName + suffix;
        newFile = dbPath + newName + suffix;
        rc = rename(oldFile.c_str(), newFile.c_str());
        if (rc != 0) return rc;
    }

    return rc;
}


//
// add new column to this table.
// If this table already has some data, set their value at this column 
// to be NULL.
// return 0 if success
// 
RC SM_TableHandle::AddColumn(string &tableName, attrInfo &colinfo)
{
    RC rc;
    string filename;

    // modify .scm file
    GetScmFile(filename, tableName);
    vector<attrInfo> attrList;
    read_scm(filename.c_str(), attrList);
    string testfile = attrList[0].name;
    attrList.push_back(colinfo);
    write_scm(filename.c_str(), attrList);
    vector<attrInfo>().swap(attrList);

    // create .data and .index file
    RM_FileHandle *this_rmfh = new RM_FileHandle;
    RM_FileHandle *rhs_rmfh = new RM_FileHandle;
    GetRMFile(filename, tableName, colinfo.name);
    rc = CreateRMFile(filename.c_str(), colinfo.length, SLOTS_PER_PAGE * colinfo.length, colinfo.type);
    if (rc != 0) return rc;

    rc = this_rmfh->OpenRMFile(filename.c_str());
    if (rc != 0) return rc;

    filename = dbPath + tableName + "." + testfile + ".data";
    rc = rhs_rmfh->OpenRMFile(filename.c_str());
    if (rc != 0) return rc;
    auto numRecs = rhs_rmfh->GetNumRecs();
    if (numRecs > 0)
    {
        printf("There are %d entries already. Their values in new column will be set to NULL.\n", numRecs);
        rc = this_rmfh->CopyNullFromOthers(*rhs_rmfh);
        if (rc != 0) return rc;
    }
    rc = this_rmfh->CloseRMFile();
    if (rc != 0) return rc;
    rc = rhs_rmfh->CloseRMFile();
    if (rc != 0) return rc;
    delete this_rmfh;
    delete rhs_rmfh;

    GetIXFile(filename, tableName, colinfo.name);
    rc = CreateIXFile(filename.c_str(), colinfo.type, colinfo.length);
    if (rc != 0) return rc;  

    return 0;
}


//
// delete column in this table.
// return 0 if success
// return -1 if no such column
//
RC SM_TableHandle::DropColumn(string &tableName, string &colName)
{
    RC rc = 0;
    string filename;

    // update .scm file
    vector<attrInfo> attrList;
    GetScmFile(filename, tableName);
    read_scm(filename.c_str(), attrList);
    auto iter = attrList.begin();
    while (iter != attrList.end() && iter->name != colName)
        iter++;
    if (iter == attrList.end())
    {
        return -1;
    }else {
        attrList.erase(iter);
    }
    write_scm(filename.c_str(), attrList);

    // delete .data and .index file
    GetRMFile(filename, tableName, colName);
    rc = DestroyRMFile(filename.c_str());
    if (rc != 0) return rc;

    GetIXFile(filename, tableName, colName);
    rc = DestroyIXFile(filename.c_str());
    if (rc != 0) return rc;

    return rc;
}


//
// rename column in this table.
// return 0 if success
// return -1 if no such column
//
RC SM_TableHandle::RenameColumn(string &tableName, string &oldName, string &newName)
{
    RC rc = 0;
    string filename;
    
    // update .scm file
    vector<attrInfo> attrList;
    GetScmFile(filename, tableName);
    read_scm(filename.c_str(), attrList);
    auto iter = attrList.begin();
    while (iter != attrList.end() && iter->name != oldName)
        iter++;
    if (iter == attrList.end())
    {
        return -1;
    }else {
        iter->name = newName;
    }
    write_scm(filename.c_str(), attrList);

    // update .data and .index file
    string oldFile, newFile;
    GetRMFile(oldFile, tableName, oldName);
    GetRMFile(newFile, tableName, newName);
    rc = rename(oldFile.c_str(), newFile.c_str());
    if (rc != 0) return rc;

    GetIXFile(oldFile, tableName, oldName);
    GetIXFile(newFile, tableName, newName);
    rename(oldFile.c_str(), newFile.c_str());
    if (rc != 0) return rc;

    return rc;
}


//
// insert entry to this table
// .data files and .index files will be updated.
// return 0 if success
// 
RC SM_TableHandle::InsertEntry(string &tableName, map<string,string> &entry, RID &_rid)
{
    RC rc = 0;
    string filename;
    vector<attrInfo> attrList;
    GetScmFile(filename, tableName);
    read_scm(filename.c_str(), attrList);

    RM_FileHandle *rmfh = new RM_FileHandle;
    IX_IndexHandle *ixfh = new IX_IndexHandle;
    for (auto iter = attrList.begin(); iter != attrList.end(); iter++)
    {            
        GetRMFile(filename, tableName, iter->name);
        rmfh->OpenRMFile(filename.c_str());
        GetIXFile(filename, tableName, iter->name);
        ixfh->OpenIndex(filename.c_str());
        if (entry.find(iter->name) == entry.end())
        {
            // NULL value
            cout << "set NULL value at " << iter->name << endl;
            rmfh->InsertRec(NULL, _rid);
        }else {
            switch (iter->type)
            {
            case INT:{
                int val = atoi(entry[iter->name].c_str());
                rc = rmfh->InsertRec((void *)&val, _rid);
                assert(rc == 0);
                rc = ixfh->InsertEntry((void *)&val, _rid);
                assert(rc == 0);
            }break;
            case FLOAT:{
                float val = atof(entry[iter->name].c_str());
                rc = rmfh->InsertRec((void *)&val, _rid);
                assert(rc == 0);
                rc = ixfh->InsertEntry((void *)&val, _rid);
                assert(rc == 0);
            }break;
            case STRING:{
                auto ptr = const_cast<char *>(entry[iter->name].c_str());
                rc = rmfh->InsertRec((void *)ptr, _rid);
                assert(rc == 0);
                rc = ixfh->InsertEntry((void *)ptr, _rid);
                assert(rc == 0);
            }break;      
            default:
                break;
            }
            
        }
        //cout << "insert at " << _rid.page << " " << _rid.slot << endl;

        ixfh->CloseIndex();
        rmfh->CloseRMFile();
    }
    delete rmfh;
    delete ixfh;

    return rc;
}


//
// delete entry in this table
// .data file and .index file will be updated.
// return 0 if success
//
RC SM_TableHandle::DeleteEntry(string &tableName, vector<RID> &rids)
{
    RC rc = 0;
    string filename;
    char *_key;
    RID _rid;
    RM_Record rec;
    vector<attrInfo> attrList;
    GetScmFile(filename, tableName);
    read_scm(filename.c_str(), attrList);

    RM_FileHandle *rmfh = new RM_FileHandle;
    IX_IndexHandle *ixfh = new IX_IndexHandle;
    for (auto iter = attrList.begin(); iter != attrList.end(); iter++)
    {            
        GetRMFile(filename, tableName, iter->name);
        rc = rmfh->OpenRMFile(filename.c_str());
        if (rc != 0) return rc;

        GetIXFile(filename, tableName, iter->name);
        rc = ixfh->OpenIndex(filename.c_str());
        if (rc != 0) return rc;

        for (int i = 0; i < rids.size(); i++)
        {
            if ((rc = rmfh->GetRec(rids[i], rec))
                || (rc = rec.GetData(_key)))
                return rc;
            if (!rec.IsNullValue()
                && (rc = ixfh->DeleteEntry(_key, rids[i])))
                return rc;
            rc = rmfh->DeleteRec(rids[i]);
            if (rc != 0) return rc;
        }
        rc = ixfh->CloseIndex();
        if (rc != 0) return rc;
        rc = rmfh->CloseRMFile();
        if (rc != 0) return rc;
    }
    delete rmfh;
    delete ixfh;

    return rc;
}


//
// Delete all records according to 'RidFile'
//
RC SM_TableHandle::DeleteEntry(string &tableName, string &RidFile)
{
    RC rc = 0;
    string filename;
    char *_key;
    RID _rid;
    RM_Record rec;
    vector<attrInfo> attrList;
    GetScmFile(filename, tableName);
    read_scm(filename.c_str(), attrList);

    RM_FileHandle *rmfh = new RM_FileHandle;
    IX_IndexHandle *ixfh = new IX_IndexHandle;
    FILE *fp = fopen(RidFile.c_str(), "r");
    for (auto iter = attrList.begin(); iter != attrList.end(); iter++)
    {            
        GetRMFile(filename, tableName, iter->name);
        rc = rmfh->OpenRMFile(filename.c_str());
        if (rc != 0) return rc;

        GetIXFile(filename, tableName, iter->name);
        rc = ixfh->OpenIndex(filename.c_str());
        if (rc != 0) return rc;

        rewind(fp);
        RID rid;
        while (fscanf(fp, "%d %d", &(rid.page), &(rid.slot)) != EOF)
        {
            if ((rc = rmfh->GetRec(rid, rec))
                || (rc = rec.GetData(_key)))
                return rc;
            if (!rec.IsNullValue()
                && (rc = ixfh->DeleteEntry(_key, rid)))
                return rc;
            rc = rmfh->DeleteRec(rid);
            if (rc != 0) return rc;
        }
        rc = ixfh->CloseIndex();
        if (rc != 0) return rc;
        rc = rmfh->CloseRMFile();
        if (rc != 0) return rc;
    }
    fclose(fp);
    delete rmfh;
    delete ixfh;

    return rc;
}


// 
// update entry which 'rid' points to in this table  
// .data files and .index files will be updated.
// return 0 if success
//
RC SM_TableHandle::UpdateEntry(string &tableName, RID rid, map<string,string> &entry)
{
    RC rc = 0;
    string filename;
    RM_Record rec;
    char *pData;
    vector<attrInfo> attrList;
    GetScmFile(filename, tableName);
    read_scm(filename.c_str(), attrList);

    RM_FileHandle *rmfh = new RM_FileHandle;
    IX_IndexHandle *ixfh = new IX_IndexHandle;
    for (auto iter = attrList.begin(); iter != attrList.end(); iter++)
    {
        if (entry.find(iter->name) == entry.end())
            continue;
        GetRMFile(filename, tableName, iter->name);
        rmfh->OpenRMFile(filename.c_str());
        GetIXFile(filename, tableName, iter->name);
        ixfh->OpenIndex(filename.c_str());
        switch (iter->type)
        {
        case INT:{
            int val = atoi(entry[iter->name].c_str());
            rmfh->GetRec(rid, rec);
            rec.GetData(pData);
            ixfh->DeleteEntry(pData, rid);
            ixfh->InsertEntry((void *)&val, rid);
            rec.Set((char *)&val, iter->length, rid);
            rmfh->UpdateRec(rec);
        }break;
        case FLOAT:{
            float val = atof(entry[iter->name].c_str());
            rmfh->GetRec(rid, rec);
            rec.GetData(pData);
            ixfh->DeleteEntry(pData, rid);
            ixfh->InsertEntry((void *)&val, rid);
            rec.Set((char *)&val, iter->length, rid);
            rmfh->UpdateRec(rec);
        }break;
        case STRING:{
            auto ptr = const_cast<char *>(entry[iter->name].c_str());
            rmfh->GetRec(rid, rec);
            rec.GetData(pData);
            ixfh->DeleteEntry(pData, rid);
            ixfh->InsertEntry((void *)ptr, rid);
            rec.Set(ptr, iter->length, rid);
            rmfh->UpdateRec(rec);
        }break;      
        default:
            break;
        }
        rmfh->CloseRMFile();
        ixfh->CloseIndex();
    }
    delete ixfh;
    delete rmfh;

    return rc;
}


RC SM_TableHandle::SelectEntry(string &tableName, string &retFile, string &column, CompOp &op, string &value)
{
    RC rc = 0;
    string filename;

    attrInfo info;
    GetScmFile(filename, tableName);
    rc = get_attr_from_scm(filename.c_str(), column, info);
    if (rc != 0) return 1;

    switch (info.type)
    {
        case STRING:{
            auto *val = const_cast<char *>(value.c_str());
            void *ptr = (void *)val;
            return this->SelectEntry(tableName, retFile, column, op, ptr);
        }break;
        case INT:{
            int val = atoi(value.c_str());
            void *ptr = (void *)&val;
            return this->SelectEntry(tableName, retFile, column, op, ptr);
        }break;
        case FLOAT:{
            float val = atof(value.c_str());
            void *ptr = (void *)&val;
            return this->SelectEntry(tableName, retFile, column, op, ptr);
        }break;
        default:
            return 0;
    }
}


//
// select entry
// write the RID of those entries which satisfy given condition 
// to temporary file
// return 0 if success
// return 1 if there is no such column
//
RC SM_TableHandle::SelectEntry(string &tableName, string &retFile, string &column, CompOp &op, void *&cmpKey)
{
    RC rc = 0;
    string filename;

    attrInfo info;
    GetScmFile(filename, tableName);
    rc = get_attr_from_scm(filename.c_str(), column, info);
    if (rc != 0) return 1;
    
    RID rid;
    GetIXFile(filename, tableName, column);
    IX_IndexHandle *ixfh = new IX_IndexHandle;
    rc = ixfh->OpenIndex(filename.c_str());
    if (rc != 0) return rc;

    rc = ixfh->OpenScan(op, cmpKey);
    if (rc != 0) return rc;
    FILE *fp = fopen(retFile.c_str(), "w");
    while (ixfh->GetNextEntry(rid) != IX_EOF)
        fprintf(fp, "%d %d\n", rid.page, rid.slot);
    fclose(fp);
    rc = ixfh->CloseScan();
    if (rc != 0) return rc;

    rc = ixfh->CloseIndex();
    if (rc != 0) return rc;
    delete ixfh;
    return rc;
}


bool compKEY(CompOp &op, void *a, void *b, AttrType type)
{
    switch (type)
    {
    case STRING: {
        int res = strcmp((char *)a, (char *)b);
        switch (op)
        {
        case EQ_OP: {
            if (res == 0) return 1;
        }break;
        case NE_OP: {
            if (res != 0) return 1;
        }break;
        case LT_OP: {
            if (res < 0) return 1;
        }break;
        case GT_OP: {
            if (res > 0) return 1;
        }break;
        case LE_OP: {
            if (res <= 0) return 1;
        }break;        
        case GE_OP: {
            if (res >= 0) return 1;
        }break;   
        }
        return 0;
    }break;
    case INT: {
        switch (op)
        {
        case EQ_OP: {
            if (*(int *)a == *(int *)b) return 1;
        }break;
        case NE_OP: {
            if (*(int *)a != *(int *)b) return 1;
        }break;
        case LT_OP: {
            if (*(int *)a < *(int *)b) return 1;
        }break;
        case GT_OP: {
            if (*(int *)a > *(int *)b) return 1;
        }break;
        case LE_OP: {
            if (*(int *)a <= *(int *)b) return 1;
        }break;        
        case GE_OP: {
            if (*(int *)a >= *(int *)b) return 1;
        }break;   
        }
        return 0;
    }break;
    case FLOAT: {
        switch (op)
        {
        case EQ_OP: {
            if (*(float *)a == *(float *)b) return 1;
        }break;
        case NE_OP: {
            if (*(float *)a != *(float *)b) return 1;
        }break;
        case LT_OP: {
            if (*(float *)a < *(float *)b) return 1;
        }break;
        case GT_OP: {
            if (*(float *)a > *(float *)b) return 1;
        }break;
        case LE_OP: {
            if (*(float *)a <= *(float *)b) return 1;
        }break;        
        case GE_OP: {
            if (*(float *)a >= *(float *)b) return 1;
        }break;   
        }
        return 0;
    }break;    
    default:
        break;
    }

    return 0;
}

//
// select entry from file
// delete RID which doesn't satisfy given condition in 'retFile' 
// return 0 if success
//
RC SM_TableHandle::SelectEntry_from_file(string &tableName, string &retFile, string &column, CompOp &op, void *&cmpKey)
{
    RC rc = 0;
    string filename;

    attrInfo info;
    GetScmFile(filename, tableName);
    rc = get_attr_from_scm(filename.c_str(), column, info);
    if (rc != 0) return 1;

    RM_Record rec;
    RID rid;
    char *pData;
    RM_FileHandle *rmfh = new RM_FileHandle;
    GetRMFile(filename, tableName, column);
    rc = rmfh->OpenRMFile(filename.c_str());
    assert(rc == 0);
    FILE *fp = fopen(retFile.c_str(), "r");
    string tmp = retFile + "x";
    FILE *fpw = fopen(tmp.c_str(), "w");
    while (fscanf(fp, "%d %d", &(rid.page), &(rid.slot)) != EOF)
    {
        rc = rmfh->GetRec(rid, rec);
        assert(rc == 0);
        rc = rec.GetData(pData);
        assert(rc == 0);

        if (compKEY(op, (void *)pData, cmpKey, info.type))
            fprintf(fpw, "%d %d\n", rid.page, rid.slot);
    }
    fclose(fpw);
    fclose(fp);
    rc = rmfh->CloseRMFile();
    assert(rc == 0);
    delete rmfh;
/* 
    rc = remove(retFile.c_str());
    assert(rc == 0);
    rc = rename(tmp.c_str(), retFile.c_str());
    assert(rc == 0); */
    return rc;
}


//
// show the column information of given table
// return 0 if success
// return 1 if no such table
//
RC SM_TableHandle::DetailTable(string &tableName)
{
    RC rc = 0;
    string filename;

    GetScmFile(filename, tableName);
    vector<attrInfo> attrList;
    int fexist = access(filename.c_str(), 0);
    if (fexist != 0) return 1;
    read_scm(filename.c_str(), attrList);

    printf("\n");
    printf("   %s  \n", tableName.c_str());
    printf("--------------------------------------\n");
    printf("NAME       TYPE         LENGTH(Byte) \n");
    printf("--------------------------------------\n");
    for (int p = 0; p < attrList.size(); p++)
        printf("%-8s  %-6s  %d \n", attrList[p].name.c_str(), attrList[p].GetAttrType().c_str(), attrList[p].length);
    printf("--------------------------------------\n");
    printf("\n");

    return rc;
}


//
// Write value at given RIDs in 'RidFile' to 'outFile'
//
RC SM_TableHandle::WriteValue(string &tableName, vector<string> &colList, string &outFile, string &RidFile)
{
    RC rc = 0;
    RM_FileHandle rmfh;
    string filename;
    RID rid;

    if (RidFile == "")
    {
        GetRMFile(filename, tableName, colList[0]);
        rmfh.OpenRMFile(filename.c_str());
        RidFile = tableName + ".where";
        rc = rmfh.WriteAllRids(RidFile.c_str());
        rmfh.CloseRMFile();
    }

    FILE *fpw = fopen(outFile.c_str(), "w"),
         *fpr = fopen(RidFile.c_str(), "r");

    fprintf(fpw, "\n");
    fprintf(fpw, "#---------------------------------------------#\n");  
    fprintf(fpw, "| ");  
    for (int c = 0; c < colList.size(); c++)
        fprintf(fpw, "%s    ", colList[c].c_str());
    fprintf(fpw, "\n");
    fprintf(fpw, "#---------------------------------------------#\n");    
    while (fscanf(fpr, "%d %d", &(rid.page), &(rid.slot)) != EOF)
    {
        fprintf(fpw, "| ");
        for (int c = 0; c < colList.size(); c++)
        {
            GetRMFile(filename, tableName, colList[c]);
            rc = rmfh.OpenRMFile(filename.c_str());
            assert(rc == 0);
            rc = rmfh.WriteValue(fpw, rid);
            assert(rc == 0);
            rc = rmfh.CloseRMFile();
            assert(rc == 0);
        }
        fprintf(fpw, "\n");
    }
    fprintf(fpw, "#---------------------------------------------#\n");    
    fclose(fpr);
    fclose(fpw);

    return rc;
}


//
// Write all records to 'outFile'
//
RC SM_TableHandle::WriteValue(string &tableName, vector<string> &colList, string &outFile)
{
    RC rc = 0;
    FILE *fp = fopen(outFile.c_str(), "w");
    RM_FileHandle rmfh;
    rmfh.OpenRMFile(tableName.c_str());
    rmfh.CloseRMFile();
    fclose(fp);
    return rc;
}


//
// check whether there is such table
//
bool SM_TableHandle::isValidTable(string &tableName)
{
    string filename;
    this->GetScmFile(filename, tableName);
    if (access(filename.c_str(), 0) == 0)
        return 1;
    return 0;
}


//
// check whether there is such column
//
bool SM_TableHandle::isValidColumn(string &tableName, string &columnName)
{
    string filename;
    this->GetScmFile(filename, tableName);
    FILE *fp = fopen(filename.c_str(), "r");
    int attrNum;
    fscanf(fp, "%d", &attrNum);
    attrInfo tmp;
    char *cname = new char[256];
    for (int i = 0; i < attrNum; i++)
    {
        fscanf(fp, "%s %d %d", 
                 cname, 
                 &(tmp.type), 
                 &(tmp.length));
        if (cname == columnName)
        {
            delete cname;
            fclose(fp);
            return 1;
        }
    }
    delete cname;
    fclose(fp);
    return 0;
}
