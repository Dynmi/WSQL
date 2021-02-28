//
// File:        ix_error.h
//


#ifndef IX_ERROR_H
#define IX_ERROR_H

#include "wsql.h"

#define IX_KEYNOTFOUND (START_IX_WARN + 0)
#define IX_INVALIDSIZE (START_IX_WARN + 1)
#define IX_ENTRYEXISTS (START_IX_WARN + 2)
#define IX_NOSUCHENTRY (START_IX_WARN + 3)
#define IX_LASTWARN IX_ENTRYEXISTS

#define IX_SIZETOOBIG      (START_IX_ERR - 0)  // key size too big
#define IX_PF              (START_IX_ERR - 1)  // error in PF
#define IX_BADIXPAGE       (START_IX_ERR - 2)  
#define IX_FCREATEFAIL     (START_IX_ERR - 3)  // record size mismatch
#define IX_HANDLEOPEN      (START_IX_ERR - 4)
#define IX_BADOPEN         (START_IX_ERR - 5)
#define IX_FNOTOPEN        (START_IX_ERR - 6)
#define IX_BADRID          (START_IX_ERR - 7)
#define IX_BADKEY          (START_IX_ERR - 8)
#define IX_EOF             (START_IX_ERR - 9)  // end of file

#define IX_LASTERROR IX_EOF

#endif
