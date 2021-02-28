//
// File:        wsql.cc
// Description: entrance for whole wsql system
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


int parse_attrList(string &cmd, vector<attrInfo> *attrList)
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
        if (ctype == ")") return -1;
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


void get_AttrType_from_scm(string &scm_file, string &colName, AttrType &type)
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


void get_CompOp_from_string(string &str, CompOp &op)
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


void get_file_list(const char *PATH, vector<string> &list)
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


//
// write database information to disk
//
void write_db_info(map<string, string> *dbList)
{
    FILE *fp = fopen(D_LIST_PATH, "w");
    fprintf(fp, "%d\n", dbList->size());
    map<string, string>::iterator iter = dbList->begin();
    while(iter != dbList->end()){
        fprintf(fp, "%s %s\n", (iter->first).c_str(), (iter->second).c_str());
        iter++;
    } 
    fclose(fp);
}


//
// get where file 
// return 0 if success
// return -1 if wrong syntax
//
int table_where(const char *cmd, string &whereFile,
            string &tbName, SM_TableHandle &th)
{
    stringstream ss;
    string column, opr, filename, value;
    ss << cmd;
    ss >> column;
    ss >> opr;
    ss >> value;
    AttrType type;
    CompOp op;
    whereFile = tbName + ".where";

    return th.SelectEntry(tbName, whereFile, column, op, value);
}


RC database_handle(const char *dbName, const char *PATH)
{
    RC rc;
    string dbPath = string(PATH) + dbName;
    SM_TableHandle th(dbPath);

    string cmd;
    while (1)
    {
        printf("WSQL@%s > ", dbName);
        getline(cin, cmd, ';');
        getchar(); 
        rc = 0;
        if (strcmp(cmd.c_str(), "list tables") == 0) {
            printf("#==================================#\n");
            printf("|           Table                  |\n");
            printf("+----------------------------------+\n");
            vector<string> files;
            get_file_list(dbPath.c_str(), files);
            for (int i = 0;i < files.size();i++)
            {
                if (files[i].find_last_of(".scm") == files[i].size()-1)
                    printf("| %-32s |\n", files[i].substr(0, files[i].size()-4).c_str());
            }           
            printf("#==================================#\n");
        }else if (strncmp(cmd.c_str(), "create table ", 13) == 0) {
            cmd = cmd.substr(13);

            // get table name
            string tablename;
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
                    tablename += cmd[i];
                i++;
            }
            if (tablename.size() == 0 || i == 0) 
                return -1;

            // get attribute list
            vector<attrInfo> attrList;
            if (l_brk != cmd.size())
            {
                string tmp = cmd.substr(tablename.size());
                parse_attrList(tmp, &attrList);
            }
            
            printf("\n------------------------------------------\n");
            printf("CREATING TABLE %s\n", tablename.c_str());
            printf("------------------------------------------\n");

            rc = th.CreateTable(tablename, &attrList);

        }else if (strncmp(cmd.c_str(), "drop table ", 11) == 0) {
            cmd = cmd.substr(11);

            // parse table name
            string tablename;
            int i = 0;
            while (i < cmd.size())
            {
                if (is_char_valid(cmd[i]))
                    tablename += cmd[i];
                i++;
            }
            if (tablename.size() == 0 || i == 0) 
                return -1;

            printf("\n------------------------------------------\n");
            printf("DROPING TABLE %s\n", tablename.c_str());
            printf("------------------------------------------\n");

            rc = th.DropTable(tablename);

        }else if (strncmp(cmd.c_str(), "alter table ", 12) == 0) {
            cmd = cmd.substr(12);
            
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
                parse_attrList(cmdstr, &attrList);

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

        }else if (strncmp(cmd.c_str(), "rename table ", 13) == 0) {
            cmd = cmd.substr(13);

            // parse old table name and new table name
            stringstream ss;
            string old_name, new_name;
            ss << cmd;
            ss >> old_name;
            ss >> new_name;
            if (!is_name_valid(old_name.c_str()) 
                || !is_name_valid(new_name.c_str()))
            {
                return -1;
            }

            rc = th.RenameTable(old_name, new_name);

        }else if (strncmp(cmd.c_str(), "update ", 7) == 0) {
            cmd = cmd.substr(7);

            string tableName;
            int colOrder;
            AttrType colType;
            stringstream ss(cmd);
            ss >> tableName;

            // get column list and value list
            string *columns = new string;
            string *values = new string;
            getline(ss, *columns, ':');
            for (int i = 0; i < columns->size(); i++)
            {
                if (!is_char_valid((*columns)[i]))
                    (*columns)[i] = ' ';
            }
            ss >> *values;
            for (int i = 0; i < values->size(); i++)
            {
                if (!is_char_valid((*values)[i]))
                    (*values)[i] = ' ';
            }
            vector<string>   colList;
            vector<string>   valList;
            string_split(&colList, *columns);
            string_split(&valList, *values);
            map<string, string> entries;
            for (int p = 0; p < colList.size(); p++)
                entries[colList[p]] = valList[p];
            vector<string>(colList).swap(colList);
            vector<string>(valList).swap(valList);
            delete columns;
            delete values;

            string whereFile;
            string cmdstr; getline(ss, cmdstr); 
            table_where(cmdstr.c_str(), whereFile, tableName, th);

            RID rid;
            FILE *fp = fopen(whereFile.c_str(), "r");
            while (fscanf(fp, "%d %d", &(rid.page), &(rid.slot)) != EOF)
            {
                rc = th.UpdateEntry(tableName, rid, entries);
                if (rc != 0) break;
            }
            fclose(fp);

        }else if (strncmp(cmd.c_str(), "delete from ", 12) == 0) {
            cmd = cmd.substr(12);

            stringstream ss(cmd);
            string tableName;
            ss >> tableName;
            
            // get where file
            string whereCondition;
            ss >> whereCondition;
            if (whereCondition != "where") return -1;
            getline(ss, whereCondition);
            string whereFile;
            table_where(whereCondition.c_str(), whereFile, tableName, th);

            vector<RID> rids;
            RID rid;
            FILE *fp = fopen(whereFile.c_str(), "r");
            while (fscanf(fp, "%d %d", &(rid.page), &(rid.slot)) != EOF)
                rids.push_back(rid);
            fclose(fp);
            rc = th.DeleteEntry(tableName, rids);

        }else if (strncmp(cmd.c_str(), "insert into ", 12) == 0) {
            cmd = cmd.substr(12);
            string tableName, whereCondition, plhd;
            stringstream ss(cmd);
            ss >> tableName;

            // get column list and value list
            string *columns = new string;
            string *values = new string;
            getline(ss, *columns, ':');
            for (int i = 0; i < columns->size(); i++)
            {
                if (!is_char_valid((*columns)[i]))
                    (*columns)[i] = ' ';
            }
            ss >> *values;
            for (int i = 0; i < values->size(); i++)
            {
                if (!is_char_valid((*values)[i]))
                    (*values)[i] = ' ';
            }
            vector<string>   colList;
            vector<string>   valList;
            string_split(&colList, *columns);
            string_split(&valList, *values);
            map<string, string> entries;
            for (int p = 0; p < colList.size(); p++)
                entries[colList[p]] = valList[p];
            vector<string>(colList).swap(colList);
            vector<string>(valList).swap(valList);
            delete columns;
            delete values;

            RID rid;
            rc = th.InsertEntry(tableName, entries, rid);

        }else if (strncmp(cmd.c_str(), "select ", 7) == 0) {
            cmd = cmd.substr(7);
            vector<string> colList;
            string tableName, tmp;
            stringstream ss;
            ss << cmd;
            ss >> tmp;
            string_split(&colList, tmp);
            ss >> tmp; if (tmp != "from");
            ss >> tableName;

            // get where file
            string whereCondition;
            ss >> whereCondition;
            if (whereCondition != "where") return -1;
            getline(ss, whereCondition);
            string whereFile;
            table_where(whereCondition.c_str(), whereFile, tableName, th);
                
            string outFile = "temp.select";
            rc = th.WriteValue(tableName, colList, outFile, whereFile);
            string syscmd = "cat " + outFile;
            system(syscmd.c_str());

        }else if (strcmp(cmd.c_str(), "exit") == 0) {
            break;
        }
    }

    return rc;
}


int main(int argc, char* argv[])
{
    string dbName;
    // load databases info
    static map<string, string> dbList;
    int fexist = access(D_LIST_PATH, 0);
    int dbNum;
    if (fexist == 0)
    {
        ifstream ifs;
        ifs.open(D_LIST_PATH);
        ifs >> dbNum;
        string dbPath;
        int i = 0;
        while (i < dbNum)
        {
            ifs >> dbName >> dbPath;
            dbList[dbName] = dbPath;
            i++;
        }
        ifs.close();
    }else {
        ofstream ofs;
        ofs.open(D_LIST_PATH);
        ofs << dbNum << "\n";
        ofs.close();
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
            string dbPath;
            dbPath.resize(256);
            dbName = cmd.substr(16);
            if (!is_name_valid(dbName.c_str())) continue;
            printf("PATH: ");scanf("%[^\n]%*c", &dbPath[0]);
            if (access(D_LIST_PATH, 0) != 0)
            {
                printf("FAILED: PATH doesn't exist! \n");
                continue;
            }
            string syscmd = "mkdir " + dbPath + dbName;
            system(syscmd.c_str());
            dbList[dbName] = dbPath;
            write_db_info(&dbList);
        }else if (strncmp(cmd.c_str(), "drop database ", 14) == 0)
        {
            dbName = cmd.substr(14);
            if (!is_name_valid(dbName.c_str())) 
            {
                printf("FAILED: invalid name! \n");
                continue;
            }
            string syscmd = "rm -rf " + dbList[dbName] + dbName;
            system(syscmd.c_str());
            dbList.erase(dbName);
            write_db_info(&dbList);
        }else if (strncmp(cmd.c_str(), "use ", 4) == 0)
        {
            dbName = cmd.substr(4);
            if (!is_name_valid(dbName.c_str()) | dbList.find(dbName) == dbList.end()) 
            {
                printf("FAILED: invalid name! \n");
                continue;
            }
            RC rc = database_handle(dbName.c_str(), dbList[dbName].c_str());
            if (rc == 0)
            {
                cout << "------------SUCCESS-------------" << endl;
            }else 
            {
                cout << "------------FAILED -------------" << endl;
            }
        }else if (strcmp(cmd.c_str(), "exit") == 0)
        {
            break;
        }
    }

    return 0;
}
