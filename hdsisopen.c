#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ems.h"                 /* EMS error reporting routines            */
#include "hds1.h"                /* Global definitions for HDS              */
#include "rec.h"                 /* Public rec_ definitions                 */
#include "rec1.h"                /* Internal rec_ definitions               */
#include "str.h"                 /* Character string import/export macros   */
#include "dat1.h"                /* Internal dat_ definitions               */
#include "dat_err.h"             /* DAT__ error code definitions            */

#include "hds.h"
#include <sys/stat.h>
#include <ctype.h>

/*==============================================*/
/* HDS_ISOPEN - CHeck if container file is open */
/*==============================================*/

int hdsIsOpen(const char *file_str, int *isopen, int *status) {


#undef context_name
#undef context_message
#define context_name "HDS_ISOPEN_ERR"
#define context_message\
        "HDS_ISOPEN: Error checking if an HDS container file is open."


   int i;
   INT start;
   size_t file_len;
   INT lfns;
   char *fns;
   struct stat statbuf;

/* Assume the file is not open */
   *isopen = 0;

/* Enter routine. */
   if (!_ok(*status)) return *status;
   hds_gl_status = DAT__OK;

/* Strip white space from the start of the file name (but leave at
   least one character, even if the string is completely blank). */
   file_len = strlen( file_str );
   for ( start = 0; start < ( file_len - 1 ); start++ ) {
      if ( !isspace( file_str[ start ] ) ) break;
   }

/* Obtain the full path name of the file.*/
   rec1_get_path( file_str + start, file_len - start, &fns, &lfns );

/* Obtain file status information. This will fail if the file does not exist.*/
    if ( _ok( hds_gl_status ) ) {
       if ( stat( fns, &statbuf ) == 0 ) {

#if __MINGW32__ || __CYGWIN__
           /* Need windows pseudo inode data */
          win_get_inodes( fns, &statbuf.st_ino, &statbuf.st_rdev );
#endif

/* If the file appears to exist, then loop to search the File Control Vector
   for any slot which is currently open and associated with  the same file. */
         for ( i = 0; i < rec_gl_endslot; i++ ) {

/* If a slot is open and the file identification matches, then we cannot*/
/* use this file name to create a new file, since it will over-write the*/
/* existing one. Report an error.*/
             if ( rec_ga_fcv[ i ].open &&
                  ( statbuf.st_ino == rec_ga_fcv[ i ].fid->st_ino ) &&
#if __MINGW32__ || __CYGWIN__
                    ( statbuf.st_rdev == rec_ga_fcv[ i ].fid->st_rdev ) &&
#endif
                  ( statbuf.st_dev == rec_ga_fcv[ i ].fid->st_dev ) )
             {
                *isopen = 1;
                  break;
             }
          }
       }
    }

   rec_deall_mem( lfns + 1, (void **) &fns );

   return hds_gl_status;
}
