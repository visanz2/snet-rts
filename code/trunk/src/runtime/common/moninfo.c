/*
 * moninfo.c
 *
 *  Created on: 29. July  2011
 *      Author: Raimund Kirner
 *
 * This file includes data types and functions to maintain monitoring information.
 * The main purpose at creation (July 2011) was to have monitoring information for
 * each S-Net record to log the overall execution time in a (sub)network from reading
 * a record until the output of all of its descendant records at the exit of this
 * (sub)network.
 *
 */
#include <assert.h>

#include "moninfo.h"
#include "memfun.h"
#include "threading.h"
#include "debug.h"
#include "string.h"

/*
 * FIXME: introduces false sharing.
 * better: get thread specific sequence ids
 * Thread-unsafe, but we don't care, as IDs are
 * composed by a task-specific id additionally
 * anyways.
 */
static unsigned int moninfo_local_id = 0; /* sequence number to create ids */


/**
 * CAUTION!
 * Keep following names consistent with header file
 */

static const char *names_event[] = {
  "EV_INPUT_ARRIVE",
  "EV_BOX_START",
  "EV_BOX_WRITE",
  "EV_BOX_FINISH",
  "EV_FILTER_START",
  "EV_FILTER_WRITE",
  "EV_SYNC_FIRST",
  "EV_SYNC_FIRE",
};

static const char *names_descr[] = {
  "MON_RECORD",
};

#define LIST_NAME MonInfoId /* SNetMonInfoIdListFUNC */
#define LIST_TYPE_NAME monid
#define LIST_VAL snet_moninfo_id_t
#define LIST_CMP SNetMonInfoCmpID
#include "list-template.c"
#undef LIST_CMP
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME



/**
 * Initialize a moninfo for a record descriptor
 *
 * Handled differently for different events
 * @pre MONINFOPTR(mon) is already allocated
 */
void MonInfoInitRec(snet_moninfo_t *mon, va_list args)
{
  assert( MONINFO_DESCR( mon) == MON_RECORD );

  snet_record_t *rec = va_arg( args, snet_record_t *);
  switch ( REC_DESCR( rec)) {
    case REC_data: /* currently only data records can be monitored this way */

      switch ( MONINFO_EVENT(mon) ) {
        case EV_INPUT_ARRIVE:
          /* signature: SNetMonInfoEvent(snet_moninfo_event_t event, snet_moninfo_descr_t, snet_record_t* rec) */
          /* action: create minimal moninfo data (no parents) */
          {
            snet_moninfo_id_t newid = SNetMonInfoCreateID();
            /* FIXME this should be set via the record interface! */
            DATA_REC( rec, id) = newid;
            DATA_REC( rec, parent_ids) = NULL;
            /* initialize fields */
            REC_MONINFO( mon, id) = newid;
            REC_MONINFO( mon, parent_ids) = NULL;
            REC_MONINFO( mon, add_moninfo_rec_data) = SNetMonInfoRecCopyAdditionalData (DATA_REC( rec, add_moninfo_rec_data));
          }
          break;
        case EV_BOX_START:
        case EV_FILTER_START:
        case EV_SYNC_FIRST:
          /* signature: SNetMonInfoEvent(snet_moninfo_event_t event, snet_moninfo_descr_t, snet_record_t* rec) */
          /* action: create moninfo data */
          {
            REC_MONINFO( mon, id) = DATA_REC( rec, id);
            REC_MONINFO( mon, parent_ids) = (DATA_REC( rec, parent_ids) != NULL) ?  SNetMonInfoIdListCopy( DATA_REC( rec, parent_ids)) : NULL;
            REC_MONINFO( mon, add_moninfo_rec_data) = SNetMonInfoRecCopyAdditionalData (DATA_REC( rec, add_moninfo_rec_data));
          }
          break;
        case EV_BOX_WRITE:
        case EV_FILTER_WRITE:
          /* signature: SNetMonInfoEvent(snet_moninfo_event_t event, snet_moninfo_descr_t descr, snet_record_t*, snet_record_t* ) */
          /* action: set single parent_id in record and create moninfo data */
          {
            snet_record_t *parent_rec = va_arg( args, snet_record_t *);
            snet_moninfo_id_t newid = SNetMonInfoCreateID();
            snet_monid_list_t *parent_id_list = SNetMonInfoIdListCreate(1, parent_rec); 

            /* set the parent id of the record */
            /* FIXME this should be set via the record interface! */
            DATA_REC( rec, parent_ids) = SNetMonInfoIdListCopy(parent_id_list);
            DATA_REC( rec, id) = newid;
            /* initialize fields */
            REC_MONINFO( mon, id) = newid;
            REC_MONINFO( mon, parent_ids) = parent_id_list;
            REC_MONINFO( mon, add_moninfo_rec_data) = SNetMonInfoRecCopyAdditionalData (DATA_REC( rec, add_moninfo_rec_data));
          }
          break;
        case EV_SYNC_FIRE:
          /* signature: SNetMonInfoEvent(snet_moninfo_event_t event,
           *                             snet_moninfo_descr_t descr,
           *                             snet_record_t* rec,
           *                             int num_patterns,
           *                             snet_record_t** storage,
           *                             snet_record_t* dummy
           *                            ) */
          /* action: set parent_id_list in record and create moninfo data */
          {
            int num_patterns = va_arg( args, int);
            snet_record_t **storage = va_arg( args, snet_record_t **);
            snet_record_t *dummy = va_arg( args, snet_record_t *);
            snet_monid_list_t *parent_id_list = SNetMonInfoIdListCreate(0);
            snet_moninfo_id_t newid = SNetMonInfoCreateID();
            int i;

            for (i=0; i<num_patterns; i++) {
              //snet_moninfo_t monid;
              if (storage[i] != NULL && storage[i] != dummy) {
                SNetMonInfoIdListAppend(parent_id_list, DATA_REC( storage[i], id));
              }
            }
            /* set the parent id of the record */
            DATA_REC( rec, parent_ids) = SNetMonInfoIdListCopy(parent_id_list);
            DATA_REC( rec, id) = newid;
            /* initialize fields */
            REC_MONINFO( mon, id) = newid;
            REC_MONINFO( mon, parent_ids) = parent_id_list;
            REC_MONINFO( mon, add_moninfo_rec_data) = SNetMonInfoRecCopyAdditionalData( DATA_REC( rec, add_moninfo_rec_data));
          }
          break;
        case EV_BOX_FINISH:
          /* action: currently just ignored... */
          break;
        default:
          SNetUtilDebugFatal("Unknown monitoring information event. [event=%d]", MONINFO_EVENT(mon));
      } /* MONINFO_EVENT( mon) */
      break;
    default:
      SNetUtilDebugFatal("Non-supported monitoring information record."
          "[event=%d][record-descr=%d]", MONINFO_EVENT(mon), REC_DESCR( rec));
  } /* REC_DESCR( rec) */

}

/*****************************************************************************
 * Create monitoring information (entries depend on monitoring item)
 ****************************************************************************/
snet_moninfo_t *SNetMonInfoCreate ( snet_moninfo_event_t event, snet_moninfo_descr_t descr,... )
{
  snet_moninfo_t *mon;
  va_list args;

  mon = SNetMemAlloc( sizeof( snet_moninfo_t));
  MONINFO_DESCR( mon) = descr;
  MONINFO_EVENT( mon) = event;

  va_start( args, descr);
  switch (descr) {
    case MON_RECORD:
      MONINFOPTR( mon) = SNetMemAlloc( sizeof( snet_moninfo_record_t));
      /* call function to initialize record monitoring specific data */
      MonInfoInitRec(mon, args);
      break;
  default:
    SNetUtilDebugFatal("Unknown monitoring information description. [%d]", descr);
    break;
  }
  va_end( args);
  return mon;
}


/*****************************************************************************
 * Delete monitoring information (entries depend on monitoring item)
 ****************************************************************************/
void SNetMonInfoDestroy( snet_moninfo_t *mon)
{
  switch (MONINFO_DESCR( mon)) {
  case MON_RECORD:
    if (REC_MONINFO( mon, parent_ids) != NULL) SNetMonInfoIdListDestroy( REC_MONINFO( mon, parent_ids));
    if (REC_MONINFO( mon, add_moninfo_rec_data) != NULL) SNetMemFree( REC_MONINFO( mon, add_moninfo_rec_data));
    SNetMemFree( MONINFOPTR( mon));
    break;
  default:
    SNetUtilDebugFatal("Unknown monitoring information description. [%d]", MONINFO_DESCR( mon));
    break;
  }
}


/*****************************************************************************
 * Create unique system-wide id
 ****************************************************************************/
snet_moninfo_id_t SNetMonInfoCreateID(void)
{
  snet_moninfo_id_t id;
  id.ids[0] = moninfo_local_id++;
  id.ids[1] = SNetThreadingGetId();
  id.ids[2] = 0;  /* FIXME: add code to get node id (distributed snet) */
  return id;
}


/*****************************************************************************
 * Compares two monitoring information identifiers
 ****************************************************************************/
bool SNetMonInfoCmpID (snet_moninfo_id_t monid1, snet_moninfo_id_t monid2)
{
   bool res = true;
   /* determine array size */
   int i;
   for (i = SNET_MONINFO_ID_VEC_SIZE-1; i>=0; i--) {
       if (monid1.ids[i] != monid2.ids[i]) {
           res = false;
           break;
       }
   }
   return res;
}


/*****************************************************************************
 * Create a copy of the additional data of record monitoring information
 ****************************************************************************/
snet_add_moninfo_rec_data_t SNetMonInfoRecCopyAdditionalData(snet_add_moninfo_rec_data_t add_data)
{
  snet_add_moninfo_rec_data_t new_add_data;
  if (add_data != NULL) {
      new_add_data = (snet_add_moninfo_rec_data_t) strdup ( add_data);
      if (new_add_data == NULL)
        SNetUtilDebugFatal("Copy of additional monitoring information for records failed. [%s]", add_data);
  }
  else {
      new_add_data = NULL;
  }
  return new_add_data;
}




static void PrintMonInfoId(FILE *f, snet_moninfo_id_t *id)
{
  fprintf(f, "(%lu.%lu.%lu)",
      id->ids[2], id->ids[1], id->ids[0]
      );
}

void SNetMonInfoPrint(FILE *f, snet_moninfo_t *mon)
{
  snet_moninfo_id_t par_id;

  fprintf(f,
      "%s,%s,",
      names_event[MONINFO_EVENT(mon)],
      names_descr[MONINFO_DESCR(mon)]
      );

  switch (mon->mon_descr) {
    case MON_RECORD: /* monitoring of a record */
      {
        PrintMonInfoId( f, &(REC_MONINFO( mon, id)) );
        fprintf(f, ",");
        if (REC_MONINFO( mon, parent_ids) != NULL) {
          LIST_FOR_EACH( REC_MONINFO( mon, parent_ids), par_id)
            PrintMonInfoId( f, &par_id );
          END_FOR
        }
        if (REC_MONINFO( mon, add_moninfo_rec_data) != NULL) {
          /*TODO*/
        }
      }
      break;
    default: /*ignore*/
      break;
  }

}
