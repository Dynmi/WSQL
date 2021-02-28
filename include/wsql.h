//
// wsql.h
//   global declarations
//
#ifndef WSQL_H
#define WSQL_H

// DO NOT include any other files in this file.

//
// Globally-useful defines
//
#define MAXNAME       24                // maximum length of a relation
                                        // or attribute name
#define MAXSTRINGLEN  1024               // maximum length of a
                                        // string-type attribute
#define MAXATTRS      40                // maximum number of attributes
                                        // in a relation

#define yywrap() 1
void yyerror(const char *);

#define SCHEMA_SUFFIX ".scm"
#define RM_SUFFIX     ".data"
#define IX_SUFFIX     ".index"

#define SUCCESS_STRING "------------SUCCESS-------------"
#define FAILED_STRING  "------------FAILED -------------"

#define D_LIST_PATH "databases.list"

//
// Return codes
//
typedef int RC;

#define OK_RC         0    // OK_RC return code is guaranteed to always be 0

#define START_PF_ERR  (-1)
#define END_PF_ERR    (-100)
#define START_RM_ERR  (-101)
#define END_RM_ERR    (-200)
#define START_IX_ERR  (-201)
#define END_IX_ERR    (-300)
#define START_SM_ERR  (-301)
#define END_SM_ERR    (-400)
#define START_QL_ERR  (-401)
#define END_QL_ERR    (-500)

#define START_PF_WARN  1
#define END_PF_WARN    100
#define START_RM_WARN  101
#define END_RM_WARN    200
#define START_IX_WARN  201
#define END_IX_WARN    300
#define START_SM_WARN  301
#define END_SM_WARN    400
#define START_QL_WARN  401
#define END_QL_WARN    500

// ALL_PAGES is defined and used by the ForcePages method defined in RM
// and PF layers
const int ALL_PAGES = -1;

//
// Attribute types
//
enum AttrType {
    NULL_TYPE,
    INT,
    FLOAT,
    STRING
};

//
// Comparison operators
//
enum CompOp {
    NO_OP,                                      // no comparison
    EQ_OP, NE_OP, LT_OP, GT_OP, LE_OP, GE_OP    // binary atomic operators
};

//
// Pin Strategy Hint
//
enum ClientHint {
    NO_HINT                                     // default value
};

#ifndef BOOLEAN
typedef char Boolean;
#endif

#ifndef NULL
#define NULL 0
#endif


#endif
