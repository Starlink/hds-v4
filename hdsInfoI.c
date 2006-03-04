
#include <stdlib.h>
#include <ctype.h>

#include "dat_err.h"
#include "hds1.h"
#include "ems.h"
#include "rec.h"
#include "str.h"
#include "dat1.h"
#include "hds.h"

/*
*+
*  Name:
*     hdsInfoI

*  Purpose:
*     Retrieve internal state from HDS as integer.

*  Invocation:
*     hdsInfoI(const HDSLoc* loc, const char * topic, const char * extra,
*              int *result, int * status);

*  Description :
*     Retrieves integer information associated with the current state
*     of the HDS internals.

*  Parameters :
*     loc = const HDSLoc* (Given)
*        HDS locator, if required by the particular topic. Will be
*        ignored for FILES and LOCATORS topics and can be NULL pointer.
*     topic = const char * (Given)
*        Topic on which information is to be obtained. Allowed values are:
*        - LOCATORS : Return the number of active locators
*        - FILES : Return the number of open files
*     extra = const char * (Given)
*        Extra options to control behaviour. The content depends on
*        the particular TOPIC. See NOTES for more information.
*     result = int* (Returned)
*        Answer to the question.
*     status = int* (Given & Returned)
*        Variable holding the status value. If this variable is not
*        SAI__OK on input, the routine will return without action.
*        If the routine fails to complete, this variable will be
*        set to an appropriate error number.

*  Return Value:
*     Returns global status on exit.

*  See Also:
*     hdsShow

*  Notes:
*     - Can be used to help debug locator leaks.
*     - The "extra" information is used by the following topics:
*       - "LOCATORS", if non-NULL, "extra" can contain a comma
*         separated list of locator paths (upper case, as returned
*         by hdsTrace) that should be included in the count. If any
*         component is preceeded by a '!' all locators with starting
*         with that path will be ignored in the count. This can be
*         used to remove parameter locators from the count.
*       - If "!EXTINCTION,EXTINCTION" is requested then they will
*         match everything, since the test is performed on each
*         component separately.

*  Authors
*     TIMJ: Tim Jenness (JAC, Hawaii)
*     {enter_new_authors_here}

*  History :
*     25-JAN-2006 (TIMJ):
*        Create from hdsShow.
*     26-JAN-2006 (TIMJ):
*        - Move into separate file.
*        - Add "extra" information
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2006 Particle Physics and Astronomy Research Council.
*     All Rights Reserved.

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 2 of
*     the License, or (at your option) any later version.
*
*     This program is distributed in the hope that it will be
*     useful, but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public
*     License along with this program; if not, write to the Free
*     Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
*     MA 02111-1307, USA

*  Bugs:
*     {note_any_bugs_here}

*-
*/

int
hdsInfoI(const HDSLoc* loc, const char *topic_str, const char * extra_str,
	 int *result, int  *status)
{        
/*===============================*/
/* HDS_INFOI - Retrieve HDS statistic */
/*===============================*/

/* Maximum number of components to use as filter */
#define MAXCOMP 20

#undef context_name
#undef context_message
#define context_name "HDS_INFOI_ERR"
#define context_message\
        "HDS_INFOI: Error retrieving HDS statistics."

   struct DSC      topic; 

   struct LCP      *lcp;
   struct LCP_DATA *data;
   char            name[DAT__SZNAM];
   struct LOC      locator;
   struct STR      path;
   struct STR      file;
   int             i;
   int             j;
   int             ncomp = 0;
   char            *comps[MAXCOMP]; 
   size_t          len;
   int             match;
   int             exclude;
   int             atstart;
   int             tracestat;
   int             nlev;
   char            extra[STR_K_LENGTH];

/* These buffers are only used when using VMS descriptors. */

#if defined( vms )
   char            pbuf[STR_K_LENGTH];
   char            fbuf[STR_K_LENGTH];
#endif

/* Enter routine        */

   *result = 0;
   if (!_ok(*status))
      return *status;
   hds_gl_status = DAT__OK;

/* Import the name of the statistic to be shown.        */

   _strcsimp(&topic,topic_str);

   /* Copy characters from the EXTRA input to the output,
      uppercasing as we go and removing spaces */
   j=0;
   if (extra_str != NULL) {
     len = strlen(extra_str);
     if (len > STR_K_LENGTH-1) {
       *status = DAT__TRUNC;
       emsSeti("E", len);
       emsSeti("M", STR_K_LENGTH-1);
       emsRep( "HDS_INFOI_1",
	       "EXTRA string exceeds maximum length (^E > ^M)",
	       status);
       return *status;
     }
     for (i=0; i<len; i++) {
       if ( extra_str[i] != ' ') {
	 extra[j] = toupper(extra_str[i]);
	 j++;
       }
     }
     extra[j] = '\0';
   }

/* Initialise strings.  */

   _strinit(&path, STR_K_LENGTH, pbuf);
   _strinit(&file, STR_K_LENGTH, fbuf);

/* Ensure that HDS has been initialised.                                    */
   if ( !hds_gl_active )
   {
      dat1_init( );
      if ( !_ok( hds_gl_status ) ) return hds_gl_status;
   }


/* Format the topic name and calculate appropriate statistic.    */
   dau_check_name(&topic, name);

   /* Number of open files */
   if (_cheql(4,name, "FILE"))
      rec_count_files( result );
   if (_cheql(4,name, "LOCA"))
   {
     /* Locators has the "extra" option to filter out paths */
     /* First see whether we have any components to filter on */
     /* If we find some we store the pointers into comps[] */
     ncomp = 0;
     if (extra_str != NULL) {
       /* use the upper case version */
       len = strlen(extra);
       /* Indicate when we are starting a string (so next pointer
	  should be stored) */
       atstart = 1;
       for (i=0; i<len; i++ ) {
	 if (extra[i] == ',') {
	   /* found a comma. Next time round we store the pointer
	      in the string (unless we have run out of space).
	      We now replace the ',' with a NUL. */
	   atstart = 1; /* next time round we start a new component */
	   extra[i] = '\0';
	 } else if (atstart) {
	   /* just in case we have ,, */
	   comps[ncomp] = &(extra[i]);
	   ncomp++;
	   atstart = 0;
	   if (ncomp >= MAXCOMP) {
	     /* run out of space */
	     *status = DAT__NOMEM;
	     emsSeti("MAX", MAXCOMP);
	     emsRep("HDSINFOI",
		    "Too many components to filter on. Max = ^MAX",
		    status);
	     return *status;
	   }
	 }
       }
     }

     /* Count number of valid locators */
      lcp          = dat_ga_wlq;
      locator.check    = DAT__LOCCHECK;
      *result = 0;
      for (i=0; i<dat_gl_wlqsize; i++)
      {
         data = &lcp->data;
	 if (data->valid) {
	   if (ncomp > 0) {
	     /* we have a list of filters so we need to trace the
		locator */
	     locator.lcp   = lcp;
	     locator.seqno = lcp->seqno;
	     tracestat = DAT__OK;
	     hdsTrace( &locator, &nlev, path.body,
		       file.body, &tracestat,
		       STR_K_LENGTH, STR_K_LENGTH);
	     if (!_ok(tracestat)) {
               *status   = tracestat;
	     } else {
	       /* Good trace - now compare and contrast */
	       /* we can match on more than one item */
	       match = 0;
	       exclude = 0;
	       for (j=0; j<ncomp; j++) {
		 /* matching or anti-matching? */
		 if ( *(comps[j]) == '!' ) {
		   /* do not forget to start one character in for the ! */
		   if (strncmp(path.body, (comps[j])+1,
			       strlen(comps[j])-1) == 0) {
		     /* Should be exempt */
		     exclude = 1;
		   }
		 } else {
		   if (strncmp(path.body, comps[j], strlen(comps[j])) == 0) {
		     /* Should be included */
		     match = 1;
		   }
		 }
	       }
	       /* increment if we either matched something
		  or was not excluded */
	       if (match || !exclude ) (*result)++;
	     }
	   } else {
	     /* quick version */
	     (*result)++;
	   }
	 }
	 lcp = lcp->flink;
      }
   }
   return hds_gl_status;
}