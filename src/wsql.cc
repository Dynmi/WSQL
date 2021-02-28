//
// File:        wsql.cc
// Description: entrance of WSQL
// Authors:     Haris Wang (dynmiw@gmail.com)
//
// 
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <vector>
#include <dirent.h>
#include <sstream>
#include <fstream>
#include <map>
#include "wsql.h"
#include "sm.h"

using namespace std;

string logostr[25] = {
"|=========================================================================================================|\n", 
"|                                                                                                         |\n",
"|  WWWWWWWW                           WWWWWWWW  SSSSSSSSSSSSSSS     QQQQQQQQQ    LLLLLLLLLLL              |\n",
"|  W::::::W                           W::::::WSS:::::::::::::::S  QQ:::::::::QQ  L:::::::::L              |\n",
"|  W::::::W                           W::::::S:::::SSSSSS::::::SQQ:::::::::::::QQL:::::::::L              |\n",
"|  W::::::W                           W::::::S:::::S     SSSSSSQ:::::::QQQ:::::::LL:::::::LL              |\n",
"|   W:::::W           WWWWW           W:::::WS:::::S           Q::::::O   Q::::::Q L:::::L                |\n",
"|    W:::::W         W:::::W         W:::::W S:::::S           Q:::::O     Q:::::Q L:::::L                |\n",
"|     W:::::W       W:::::::W       W:::::W   S::::SSSS        Q:::::O     Q:::::Q L:::::L                |\n",
"|      W:::::W     W:::::::::W     W:::::W     SS::::::SSSSS   Q:::::O     Q:::::Q L:::::L                |\n",
"|       W:::::W   W:::::W:::::W   W:::::W        SSS::::::::SS Q:::::O     Q:::::Q L:::::L                |\n",
"|        W:::::W W:::::W W:::::W W:::::W            SSSSSS::::SQ:::::O     Q:::::Q L:::::L                |\n",
"|         W:::::W:::::W   W:::::W:::::W                  S:::::Q:::::O  QQQQ:::::Q L:::::L                |\n",
"|          W:::::::::W     W:::::::::W                   S:::::Q::::::O Q::::::::Q L:::::L         LLLLLL |\n",
"|           W:::::::W       W:::::::W        SSSSSSS     S:::::Q:::::::QQ::::::::LL:::::::LLLLLLLLL:::::L |\n",
"|            W:::::W         W:::::W         S::::::SSSSSS:::::SQQ::::::::::::::QL::::::::::::::::::::::L |\n",
"|             W:::W           W:::W          S:::::::::::::::SS   QQ:::::::::::Q L::::::::::::::::::::::L |\n",
"|              WWW             WWW            SSSSSSSSSSSSSSS       QQQQQQQQ::::QLLLLLLLLLLLLLLLLLLLLLLLL |\n",
"|                                                                           Q:::::Q                       |\n",
"|                                                                            QQQQQQ                       |\n",
"|                                                                                                         |\n",
"|=========================================================================================================|\n",
"                                                                                                           \n",                                                                                
"                           Copyright (c) 2021, Haris Wang(dynmiw@gmail.com)                                \n",
"                                                                                                           \n"
};


void inline clear_screen()
{
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    string syscmd = "cls";
#elif __APPLE__
    // apple
#elif __linux__
    string syscmd = "clear";
#else
#   error "Unknown compiler"
#endif

    system(syscmd.c_str());
}


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


bool is_char_valid(const char c)
{

    if ((c <= 'Z' && c >= 'A')
        || (c <= 'z' && c >= 'a')
        || (c <= '9' && c >= '0'))
    {
        return 1;
    }else {
        return 0;
    }
}


bool is_name_valid(const char *name)
{
    int i = 0;
    if (i == strlen(name))
        return 0;
    while (i < strlen(name))
    {
        if(!is_char_valid(name[i]))
            break;
        i++;
    }
    if (i != strlen(name))
        return 0;
    return 1;
}


void string_split(vector<string> *outList, const string &str, string delm = "(), ")
{
    char tmp[str.size()+5] = "";
    memcpy(tmp, str.c_str(), str.size());
    char *p = strtok(tmp, delm.c_str());
    while (p && p != " ")
    {
        outList->push_back(p);
        p = strtok(NULL, delm.c_str());
    }
}


void string_to_CompOp(string &str, CompOp &op)
{
    if (str == "=")
    {
        op = EQ_OP;
    }else if (str == ">")
    {
        op = GE_OP;
    }else if (str == "<")
    {
        op = LT_OP;
    }else if (str == "<=")
    {
        op = LE_OP;
    }else if (str == ">=")
    {
        op = GE_OP;
    }else if (str == "!=")
    {
        op = NE_OP;
    }
}


void string_to_attrList(string &cmd, vector<attrInfo> *attrList)
{
    // normalize the command string
    for (int i = 0; i < cmd.size(); i++)
    {
        if (cmd[i] == ' '
            || cmd[i] == '[' 
            || cmd[i] == ']')
        {
            continue;
        }else if (!is_char_valid(cmd[i]))
        {
            cmd[i] = ' ';
        }
    }
    cmd += " )";
    
    // parse the attribute list
    stringstream ss(cmd);
    attrInfo attrinfo;
    string cname, ctype;
    while (1)
    {
        ss >> cname;
        if (cname == ")") break;
        ss >> ctype;
        if (ctype == ")") break;
        attrinfo.name = cname;
        if (strncmp(ctype.c_str(), "INT", 3) == 0)
        {
            attrinfo.type = INT;
            attrinfo.length = 4;
        }else if (strncmp(ctype.c_str(), "FLOAT", 5) == 0)
        {
            attrinfo.type = FLOAT;
            attrinfo.length = 4;
        }else if (strncmp(ctype.c_str(), "STRING", 6) == 0)
        {
            attrinfo.type = STRING;
            attrinfo.length = stoi(ctype.substr(7, ctype.size()-8));
        }            
        attrList->push_back(attrinfo);
    }
}


void load_AttrType_from_scm(string &scm_file, string &colName, AttrType &type)
{
    FILE *fp = fopen(scm_file.c_str(), "r");
    int attrNum;
    fscanf(fp, "%d", &attrNum);
    char name[256];
    int a,b,c;
    for (int i = 0; i < attrNum; i++)
    {
        fscanf(fp, "%s %d %d %d", name, &a, &b, &c);
        if (name == colName)
        {
            type = (AttrType)a;
            break;
        }
    }
    fclose(fp);
}


void load_columns_from_scm(string &scm_file, vector<string> &colList)
{
    FILE *fp = fopen(scm_file.c_str(), "r");
    int attrNum;
    fscanf(fp, "%d", &attrNum);
    char name[256];
    int a,b,c;
    for (int i = 0; i < attrNum; i++)
    {
        fscanf(fp, "%s %d %d %d", name, &a, &b, &c);
        colList.push_back(name);
    }
    fclose(fp);
}


//
// get where file 
// return 0 if success
// return -1 if wrong syntax
//
RC table_where(string &cmd, string &whereFile,
            string &tableName, SM_TableHandle &th)
{
    stringstream ss(cmd);
    string column, opr, filename, value;
    ss >> column;
    ss >> opr; 
    ss >> value;
    CompOp op;
    string_to_CompOp(opr, op);
    
    whereFile = tableName + ".where";
    return th.SelectEntry(tableName, whereFile, column, op, value);
}


RC dml_detail_table(string &cmd, SM_TableHandle &th)
{
    stringstream ss(cmd);
    string tableName;
    ss >> tableName;
    if (!th.isValidTable(tableName))
        return 1;

    return th.DetailTable(tableName);
}


RC dml_create_table(string &cmd, SM_TableHandle &th)
{
    // get table name
    string tableName;
    int l_brk = 0;
    while (l_brk < cmd.size())
    {
        if (cmd[l_brk] == '(')
            break;
        l_brk++;
    }
    int i = 0;
    while (i < l_brk)
    {
        if (is_char_valid(cmd[i]))
            tableName += cmd[i];
        i++;
    }
    if (tableName.size() == 0 || i == 0) 
        return -1;

    // get attribute list
    vector<attrInfo> attrList;
    if (l_brk != cmd.size())
    {
        string tmp = cmd.substr(tableName.size());
        string_to_attrList(tmp, &attrList);
    }

    printf("\n------------------------------------------\n");
    printf("CREATING TABLE %s\n", tableName.c_str());
    printf("------------------------------------------\n");

    if (attrList.size() == 0)
    {
        return th.CreateTable(tableName, NULL);
    }else {
        return th.CreateTable(tableName, &attrList);
    }
}


RC dml_drop_table(string &cmd, SM_TableHandle &th)
{
    stringstream ss(cmd);
    string tableName;
    ss >> tableName;
    if (!th.isValidTable(tableName))
        return 1;

    printf("\n------------------------------------------\n");
    printf("DROPING TABLE %s\n", tableName.c_str());
    printf("------------------------------------------\n");

    return th.DropTable(tableName);
}


RC dml_alter_table(string &cmd, SM_TableHandle &th)
{
    RC rc = 0;
    // parse table name and operation
    for (int i = 0; i < cmd.size(); i++)
    {
        if (cmd[i] == ' ' 
            || cmd[i] == '('
            || cmd[i] == ')'
            || cmd[i] == '['
            || cmd[i] == ']') 
            continue;
        if (!is_char_valid(cmd[i]))
            return -1;         
    }
    stringstream ss(cmd);
    string tableName, operation, colName, filename;
    ss >> tableName;
    ss >> operation;    
    ss >> colName; if (colName != "column"){ return -1;} 

    printf("\n------------------------------------------\n");
    printf("ALTERING TABLE %s  %s\n", tableName.c_str(), operation.c_str());
    printf("------------------------------------------\n");

    if (operation == "rename")
    {
        string newcolName;
        ss >> colName;
        ss >> newcolName;
        rc = th.RenameColumn(tableName, colName, newcolName);

    }else if (operation == "add")
    {
        vector<attrInfo> attrList;
        string cmdstr; getline(ss, cmdstr);    
        string_to_attrList(cmdstr, &attrList);
        for (auto iter = attrList.begin(); iter != attrList.end(); iter++)
        {
            rc = th.AddColumn(tableName, *iter);
            if (rc != 0) return rc;
        }

    }else if (operation == "drop")
    {
        ss >> colName;
        rc = th.DropColumn(tableName, colName);
    }

    return rc;
}


RC dml_clear_table(string &cmd, SM_TableHandle &th)
{
    stringstream ss(cmd);
    string tableName;
    ss >> tableName;
    if (!th.isValidTable(tableName))
        return 1;

    printf("\n------------------------------------------\n");
    printf("CLEARING TABLE %s\n", tableName.c_str());
    printf("------------------------------------------\n");

    return th.ClearTable(tableName);
}


//
// return 1 if invalid table name
//
RC dml_rename_table(string &cmd, SM_TableHandle &th)
{
    // parse old table name and new table name
    stringstream ss(cmd);
    string oldName, newName;
    ss >> oldName;
    ss >> newName;
    if (!th.isValidTable(oldName)
        || !is_name_valid(oldName.c_str()) 
        || !is_name_valid(newName.c_str()))
        return 1;

    return th.RenameTable(oldName, newName);
}


//
// return 0 if success
// return 1 if invalid table name
// return 2 if invalid column-value pair
//
RC dml_update(string &cmd, SM_TableHandle &th)
{
    RC rc = 0;
    string tableName;
    int colOrder;
    AttrType colType;
    stringstream ss(cmd);
    ss >> tableName;
    if (!th.isValidTable(tableName))
        return 1;

    // get column list and value list
    string *colStr = new string;
    string *valStr = new string;
    getline(ss, *colStr, ':');
    getline(ss, *valStr);
    *colStr = colStr->substr(1);
    if (((*colStr)[0] != '(')
        || ((*colStr)[colStr->size()-1] != ')')
        || ((*valStr)[0] != '(')
        || ((*valStr)[valStr->size()-1] != ')'))
        return 2;

    for (int i = 0; i < colStr->size(); i++)
    {
        if (!is_char_valid((*colStr)[i]))
            (*colStr)[i] = ' ';
    }
    for (int i = 0; i < valStr->size(); i++)
    {
        if (!is_char_valid((*valStr)[i]))
            (*valStr)[i] = ' ';
    }
    vector<string>   colList;
    vector<string>   valList;
    string_split(&colList, *colStr);
    string_split(&valList, *valStr);
    if (colList.size() != valList.size())
        return 2;
    map<string, string> entries;
    for (int p = 0; p < colList.size(); p++)
    {
        if (!th.isValidColumn(tableName, colList[p]))
            return 2;
        entries[colList[p]] = valList[p];
    }
    delete colStr;
    delete valStr;

    string whereFile;
    string cmdstr; getline(ss, cmdstr); 
    rc = table_where(cmdstr, whereFile, tableName, th);
    if (rc != 0) return 0;

    RID rid;
    FILE *fp = fopen(whereFile.c_str(), "r");
    while (fscanf(fp, "%d %d", &(rid.page), &(rid.slot)) != EOF)
    {
        rc = th.UpdateEntry(tableName, rid, entries);
        if (rc != 0) break;
    }
    fclose(fp);

    return rc;
}


//
// return 0 if success
// return 1 if invalid table name
// return 2 if invalid column-value pair
//
RC dml_insert(string &cmd, SM_TableHandle &th)
{
    stringstream ss(cmd);
    string tableName, whereCondition, plhd;
    ss >> tableName;
    if (!th.isValidTable(tableName))
        return 1;

    // get column list and value list
    string *colStr = new string;
    string *valStr = new string;
    getline(ss, *colStr, ':');
    getline(ss, *valStr);
    *colStr = colStr->substr(1);
    if (((*colStr)[0] != '(')
        || ((*colStr)[colStr->size()-1] != ')')
        || ((*valStr)[0] != '(')
        || ((*valStr)[valStr->size()-1] != ')'))
        return 2;

    for (int i = 0; i < colStr->size(); i++)
    {
        if (!is_char_valid((*colStr)[i]))
            (*colStr)[i] = ' ';
    }
    for (int i = 0; i < valStr->size(); i++)
    {
        if (!is_char_valid((*valStr)[i]))
            (*valStr)[i] = ' ';
    }
    vector<string>   colList;
    vector<string>   valList;
    string_split(&colList, *colStr);
    string_split(&valList, *valStr);
    if (colList.size() != valList.size())
        return 2;
    map<string, string> entries;
    for (int p = 0; p < colList.size(); p++)
    {
        if (!th.isValidColumn(tableName, colList[p]))
            return 2;
        entries[colList[p]] = valList[p];
    }
    delete colStr;
    delete valStr;

    RID rid;
    return th.InsertEntry(tableName, entries, rid);
}


//
// return 0 if success
// return 1 if invalid table name
//
RC dml_delete(string &cmd, SM_TableHandle &th)
{
    RC rc = 0;
    stringstream ss(cmd);
    string tableName, plhd, whereCondition;
    ss >> tableName;
    if (!th.isValidTable(tableName))
        return 1;
    
    // get where file
    ss >> plhd; if (plhd != "where"){return -1;}
    getline(ss, whereCondition);
    string whereFile;
    rc = table_where(whereCondition, whereFile, tableName, th);
    if (rc != 0) return 0;

    return th.DeleteEntry(tableName, whereFile);
}


//
// return 0 if success
// return 1 if invalid table name
// return 2 if invalid column name
//
RC dml_select(string &cmd, SM_TableHandle &th)
{
    RC rc = 0;
    string tableName, colstr, plhd;
    stringstream ss;
    ss << cmd;
    ss >> colstr;
    ss >> plhd; if (plhd != "from"){return 1;}
    ss >> tableName;
    if (!th.isValidTable(tableName))
        return 1;

    vector<string> colList;
    if (colstr == "*")
    {
        string filename;
        th.GetScmFile(filename, tableName);
        load_columns_from_scm(filename, colList);
    }else {
        string_split(&colList, colstr);
        if (colList.size() == 0)
            return 2;
        for (int p = 0; p < colList.size(); p++)
        {
            if (!th.isValidColumn(tableName, colList[p]))
                return 2;
        }
    }

    printf("\n------------------------------------------\n");
    printf("SELECT FROM TABLE %s\n", tableName.c_str());
    printf("------------------------------------------\n");

    string outFile = tableName + ".select";

    string whereCondition, whereFile;
    ss >> plhd;
    if (plhd == "where")
    {
        getline(ss, whereCondition);
        rc = table_where(whereCondition, whereFile, tableName, th);
        if (rc != 0) return 0;
        rc = th.WriteValue(tableName, colList, outFile, whereFile);
    }else {
        // no where condition is given
        whereFile = "";
        rc = th.WriteValue(tableName, colList, outFile, whereFile);
        if (rc == 1) return rc;
    }

#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    string syscmd = "type " + outFile; 
#elif __APPLE__
    // apple
#elif __linux__
    string syscmd = "cat " + outFile;
#else
#   error "Unknown compiler"
#endif

    system(syscmd.c_str());
    return rc;
}


void database_handle(string &dbName, string &dbPath)
{
    SM_TableHandle th(dbPath);
    string cmd;
    while (1)
    {
        printf("WSQL@%s > ", dbName.c_str());
        getline(cin, cmd, ';');
        getchar(); 
        RC rc = 999;
        if (strcmp(cmd.c_str(), "list tables") == 0) 
        {
            printf("#==================================#\n");
            printf("|           Table                  |\n");
            printf("+----------------------------------+\n");
            vector<string> files;
            list_files(dbPath.c_str(), files);
            for (int i = 0;i < files.size();i++)
            {
                if (files[i].find_last_of(".scm") == files[i].size()-1)
                    printf("| %-32s |\n", files[i].substr(0, files[i].size()-4).c_str());
            }           
            printf("#==================================#\n");
        }else if (strncmp(cmd.c_str(), "detail table ", 13) == 0) 
        {
            cmd = cmd.substr(13);
            rc = dml_detail_table(cmd, th);
        }else if (strncmp(cmd.c_str(), "create table ", 13) == 0) 
        {
            cmd = cmd.substr(13);
            rc = dml_create_table(cmd, th);
        }else if (strncmp(cmd.c_str(), "drop table ", 11) == 0) 
        {
            cmd = cmd.substr(11);
            rc = dml_drop_table(cmd, th);
        }else if (strncmp(cmd.c_str(), "alter table ", 12) == 0) 
        {
            cmd = cmd.substr(12);
            rc = dml_alter_table(cmd, th);
        }else if (strncmp(cmd.c_str(), "clear table ", 12) == 0) 
        {
            cmd = cmd.substr(12);
            rc = dml_clear_table(cmd, th);
        }else if (strncmp(cmd.c_str(), "rename table ", 13) == 0) 
        {
            cmd = cmd.substr(13);
            rc = dml_rename_table(cmd, th);
        }else if (strncmp(cmd.c_str(), "update ", 7) == 0)
        {
            cmd = cmd.substr(7);
            rc = dml_update(cmd, th);
        }else if (strncmp(cmd.c_str(), "delete from ", 12) == 0) 
        {
            cmd = cmd.substr(12);
            rc = dml_delete(cmd, th);
        }else if (strncmp(cmd.c_str(), "insert into ", 12) == 0) 
        {
            cmd = cmd.substr(12);
            rc = dml_insert(cmd, th);
        }else if (strncmp(cmd.c_str(), "select ", 7) == 0) 
        {
            cmd = cmd.substr(7);
            rc = dml_select(cmd, th);
        }else if (strcmp(cmd.c_str(), "exit") == 0) 
        {
            break;
        }else if (strcmp(cmd.c_str(), "clear") == 0) 
        {
            clear_screen();
        }

        if (rc == 0)
        {
            cout << SUCCESS_STRING << endl;
        }else if (rc != 999)
        {
            cout << FAILED_STRING << " Error code " << rc << endl;
        }
    }
}


//
// load database information from disk
//
void load_dbInfo(map<string, string> &dbList)
{
    ifstream ifs;
    ifs.open(D_LIST_PATH);
    int dbNum;
    ifs >> dbNum;
    string dbName, dbPath;
    int i = 0;
    while (i < dbNum)
    {
        ifs >> dbName >> dbPath;
        dbList[dbName] = dbPath;
        i++;
    }
    ifs.close();
}


//
// write database information to disk
//
void write_dbInfo(map<string, string> &dbList)
{
    ofstream ofs;
    ofs.open(D_LIST_PATH);
    ofs << dbList.size() << "\n";
    auto iter = dbList.begin();
    while (iter != dbList.end())
    {
        ofs << iter->first << " " << iter->second << "\n";
        iter++;
    }
    ofs.close();
}


int main(int argc, char* argv[])
{
    // load databases info
    static map<string, string> dbList;
    int dbNum;
    if (access(D_LIST_PATH, 0) == 0)
    {
        load_dbInfo(dbList);
        dbNum = dbList.size();
    }else {
        write_dbInfo(dbList);
    }

    cout << endl;
    for(int i = 0; i < 26; i++) cout << logostr[i];
    cout << endl;

    // interact with user
    string cmd;
    while (1)
    {
        printf("WSQL > ");
        getline(cin, cmd, ';');
        getchar();
        if (strcmp(cmd.c_str(), "list databases") == 0)
        {
            printf("#==================================#\n");
            printf("|           DATABASE               |\n");
            printf("+----------------------------------+\n");
            map<string, string>::iterator iter = dbList.begin();
            while(iter != dbList.end()){
                printf("| %-32s |\n", (iter->first).c_str());
                iter++;
            }
            printf("#==================================#\n");
        }else if (strncmp(cmd.c_str(), "create database ", 15) == 0)
        {
            string dbName;
            dbName = cmd.substr(16);
            if (!is_name_valid(dbName.c_str())
                || dbList.find(dbName) != dbList.end())
                continue;
            string dbPath;
            dbPath.resize(256);
            printf("PATH: ");
            getline(cin, dbPath);
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
            dbPath = dbPath + dbName + "\\";
#elif __APPLE__
    // apple
#elif __linux__
            dbPath = dbPath + dbName + "/";
#else
#   error "Unknown compiler"
#endif
            string syscmd = "mkdir " + dbPath;
            if (access(D_LIST_PATH, 0) != 0
                || system(syscmd.c_str()) != 0)
            {
                cout << FAILED_STRING << endl;
                continue;
            }        
            dbList[dbName] = dbPath;
            write_dbInfo(dbList);
            cout << SUCCESS_STRING << endl;
        }else if (strncmp(cmd.c_str(), "drop database ", 14) == 0)
        {
            string dbName;
            dbName = cmd.substr(14);

#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
            string syscmd = "rmdir /S " + dbList[dbName];
#elif __APPLE__
    // apple
#elif __linux__
            string syscmd = "rm -rf " + dbList[dbName];
#else
#   error "Unknown compiler"
#endif

            if (!is_name_valid(dbName.c_str())
                || system(syscmd.c_str()) != 0) 
            {
                cout << FAILED_STRING << endl;
                continue;
            }
            dbList.erase(dbName);
            write_dbInfo(dbList);
            cout << SUCCESS_STRING << endl;
        }else if (strncmp(cmd.c_str(), "use ", 4) == 0)
        {
            string dbName;
            dbName = cmd.substr(4);
            if (!is_name_valid(dbName.c_str()) 
                || dbList.find(dbName) == dbList.end()) 
            {
                cout << FAILED_STRING << endl;
                continue;
            }
            database_handle(dbName, dbList[dbName]);
        }else if (strcmp(cmd.c_str(), "exit") == 0)
        {
            break;
        }else if (strcmp(cmd.c_str(), "clear") == 0) 
        {
            clear_screen();
        }
    }

    return 0;
}
