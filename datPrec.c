
#include "hds1.h"
#include "sae_par.h"
#include "hds.h"
#include "dat_par.h"
#include <string.h>
#include <stdio.h>

typedef struct HDSTypes {
  size_t nbytes;
  char type[DAT__SZTYP+1];
} HDSTypes;

int datPrec( const HDSLoc * loc, size_t *nbytes, int *status ) {
/*
*  Name:
*     datPrec

*  Purpose:
*     Enquire the storage-precision for primitive object

*  Language:
*     Starlink ANSI C

*  Description:
*     This routine returns the number of basic machine units (bytes)
*     used to store a single value of the specified data object.

*  Invocation :
*     datPrec( HDSLoc * loc, size_t * nbytes, int * status );
*     CALL DAT_PREC(LOC, NBYTES, STATUS)

*  Parameters :
*    loc = HDSLoc * (Given)
*    LOC = _CHAR*(DAT__SZLOC) (Given)
*        Pointer to HDS Locator associated with a primitive data object.
*    nbytes = size_t * (Returned)
*    NBYTES = INTEGER (Returned)
*        Pointer to integer to receive the number of machine units (bytes)
*        needed to store a single data value. For strings this will return
*        the number of bytes allocated to the string.
*    status = int * (Given & Returned)
*    STATUS = INTEGER (Given & Returned)
*        Variable holding the status value. If this variable is not
*        SAI__OK on input, the routine will return without action.
*        If the routine fails to complete, this variable will be
*        set to an appropriate error number.

*  Returned Value:
*     int = Global status

*  Authors :
*     Jack Giddings (UCL::JRG)
*     Dennis Kelly (REVAD::BDK)
*     Alan Chipperfield (RAL::AJC)
*     Tim Jenness (JAC, Hawaii)
*     {enter_new_authors_here}

*  History:
*     03-JAN-1983 (UCL::JRG):
*        Original.
*     05-NOV-1984 (REVAD::BDK):
*        Calls to error system removed
*     15-APR-1987: (RAL::AJC)
*        Improved prologue layout
*     28-NOV-2005 (TIMJ):
*        Rewrite in C (from dat_prec.f)
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}
*/

  /* Define the mapping from type string to size */

  HDSTypes lut[] = {
    { 8, "_DOUBLE" },
    { 4, "_REAL" }, 
    { 4, "_INTEGER" }, 
    { 4, "_LOGICAL" },
    { 1, "_CHAR" },
    { 2, "_UWORD" },
    { 2, "_WORD" },
    { 1, "_UBYTE" },
    { 1, "_BYTE" },
    { 0, "" }    /* Terminator */
  };

  /* Local variables */
  int found = 0;            /* have we found a match? */
  int i;                    /* Loop index */
  char type[DAT__SZTYP+1];  /* Actual type of primitive object */

  *nbytes = 0;
  if ( *status != SAI__OK ) return *status;

  /* Get object type - assume this has to be nul-terminated */
  datType( loc, type, status );

  if (*status == SAI__OK) {
    /* Go through type list */
    for (i = 0; lut[i].nbytes != 0 ; i++) {
      if ( strcmp( type, lut[i].type ) == 0 ) {
	found = 1;
	*nbytes = lut[i].nbytes;
	break;
      }
    }

    /* If no match was found, ask for the length of string */
    if (!found) {
      datLen( loc, nbytes, status );
    }

  }

  return *status;
}