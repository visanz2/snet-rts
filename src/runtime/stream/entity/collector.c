#include <stdlib.h>
#include <assert.h>

#include "collector.h"
#include "snetentities.h"
#include "locvec.h"
#include "memfun.h"
#include "bool.h"
#include "debug.h"

#include "threading.h"


#define LIST_NAME_H Stream
#define LIST_TYPE_NAME_H stream
#define LIST_VAL_H snet_stream_t *
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H
//#define DEBUG_PRINT_GC


/*
 * Streams that are in the waitingset are not inspected until
 * the waitingset becomes the readyset again.
 * It is possible that the next record on a stream in the waitingset
 * is a termination record, which means that the stream is not required
 * anymore and could be destroyed.
 *
 * Following flag causes a scanning of the streams in the waiting set
 * upon arrival of a collect record to find and destroy such streams that
 * are not required anymore.
 */
#define DESTROY_TERM_IN_WAITING_UPON_COLLECT

typedef struct {
  snet_stream_t *output;
  snet_stream_t *cur;
  snet_record_t *sort_rec;
  snet_record_t *term_rec;
  snet_stream_list_t * ready_list, *waiting_list;
  int cur_index;
  bool is_ready;
  int incount;
  int n;
  bool is_static;
  union {
    struct {
      int num;
      snet_stream_t **inputs;
    } st;
    struct {
      snet_stream_t *input;
    } dyn;
  } in;
} coll_arg_t;

#define CARG_ST( carg,name) (carg->in.st.name)
#define CARG_DYN(carg,name) (carg->in.dyn.name)


//FIXME in threading layer?
#define SNetStreamsetIsEmpty(set) ((*set)==NULL)



/*****************************************************************************/
/* Helper functions                                                          */
/*****************************************************************************/

static int FillList(snet_stream_list_t *streamlist, snet_streamset_t *streamset, snet_stream_desc_t *cur_stream)
{
  int cur = -1;
  int i = 0;
  snet_stream_iter_t *iter = SNetStreamIterCreate(streamset);
  while(SNetStreamIterHasNext(iter)){
    snet_stream_desc_t *tmp = SNetStreamIterNext(iter);
    if(tmp != NULL) {
      snet_stream_t *tmp_stream = SNetStreamGet(tmp);
      SNetStreamListAppendEnd(streamlist,tmp_stream);
      if(tmp == cur_stream) {
        cur = i;
      }
      SNetStreamClose(tmp,false);
      i++;
    }
  }
  SNetStreamIterDestroy(iter);
  return cur;
}


static snet_streamset_t DequeueStreamList(snet_stream_list_t *list, snet_stream_desc_t **cur_stream, int index)
{
  int i = 0;
  snet_streamset_t streamset = NULL;
  snet_stream_t * stream;
  LIST_DEQUEUE_EACH(list, stream) {
    if(stream == NULL) {
    }else{
      snet_stream_desc_t *tmp;
      tmp = SNetStreamOpen(stream, 'r');
      if(i == index){
        *cur_stream = tmp;
      }
      SNetStreamsetPut(&streamset, tmp);
    }
    i++;
  }
  return streamset;
}


/**
 * @pre rec1 and rec2 are sort records
 */
static bool SortRecEqual( snet_record_t *rec1, snet_record_t *rec2)
{
  return (SNetRecGetLevel(rec1) == SNetRecGetLevel(rec2)) &&
         (SNetRecGetNum(  rec1) == SNetRecGetNum(  rec2));
}


/**
 * Peeks the top record of all streams in waitingset,
 * if there is a terminate record, the stream is closed
 * and removed from the waitingset
 */
static int DestroyTermInWaitingSet(snet_stream_iter_t *wait_iter,
    snet_streamset_t *waitingset)
{
  int destroy_cnt=0;
  if ( !SNetStreamsetIsEmpty( waitingset)) {
    SNetStreamIterReset( wait_iter, waitingset);
    while( SNetStreamIterHasNext( wait_iter)) {
      snet_stream_desc_t *sd = SNetStreamIterNext( wait_iter);
      snet_stream_desc_t *tmp = SNetStreamOpen(SNetStreamGet(sd), 'r');
      snet_record_t *wait_rec = SNetStreamPeek( tmp);

      /* for this stream, check if there is a termination record next */
      if ( wait_rec != NULL &&
          SNetRecGetDescriptor( wait_rec) == REC_terminate ) {
        /* consume, remove from waiting set and free the stream */
        (void) SNetStreamRead( tmp);
        SNetStreamIterRemove( wait_iter);
        SNetStreamClose( sd, true);
        /* update destroyed counter */
        destroy_cnt++;
        /* destroy record */
        SNetRecDestroy( wait_rec);
      }
      SNetStreamClose( tmp, false);
      /* else do nothing */
    }
  }
  return destroy_cnt;
}


/**
 * Get a record due to the collector setting
 */
static snet_record_t *GetRecord(
    snet_streamset_t *readyset,
    int incount,
    snet_stream_desc_t **cur_stream)
{
  snet_record_t * rec;
  assert(incount >= 1 && *readyset != NULL);
  if (*cur_stream == NULL || SNetStreamPeek(*cur_stream) == NULL) {
    if (incount == 1) {
      *cur_stream = *readyset;
    } else {
      *cur_stream = SNetStreamPoll(readyset);
    }
  }
  rec = SNetStreamRead(*cur_stream);
  return rec;
}


/**
 * Process a sort record
 */
static void ProcessSortRecord(
    snet_entity_t *ent,
    snet_record_t *rec,
    snet_record_t **sort_rec,
    snet_stream_desc_t *cur_stream,
    snet_streamset_t *readyset,
    snet_streamset_t *waitingset)
{
  int res;
  assert( REC_sort_end == SNetRecGetDescriptor( rec) );

  /* sort record: place node in waitingset */
  /* remove node */
  res = SNetStreamsetRemove( readyset, cur_stream);
  assert(res == 0);
  /* put in waiting set */
  SNetStreamsetPut( waitingset, cur_stream);

  if (*sort_rec!=NULL) {
    /*
     * check that level & counter match
     */
    if( !SortRecEqual(rec, *sort_rec) ) {
      SNetUtilDebugNoticeEnt( ent,
          "[COLL] Warning: Received sort records do not match! "
          "expected (l%d,c%d) got (l%d,c%d) on %p", /* *trollface* PROBLEM? */
          SNetRecGetLevel(*sort_rec), SNetRecGetNum(*sort_rec),
          SNetRecGetLevel(rec),       SNetRecGetNum(rec),
          cur_stream
          );
    }
    /* destroy record */
    SNetRecDestroy( rec);
  } else {
    *sort_rec = rec;
  }
}


/**
 * Process a termination record
 */
static void ProcessTermRecord(
    snet_record_t *rec,
    snet_record_t **term_rec,
    snet_stream_desc_t *cur_stream,
    snet_streamset_t *readyset,
    int *incount)
{
  assert( REC_terminate == SNetRecGetDescriptor( rec) );
  int res = SNetStreamsetRemove( readyset, cur_stream);
  assert(res == 0);
  SNetStreamClose( cur_stream, true);
  /* update incoming counter */
  *incount -= 1;

  if (*term_rec == NULL) {
    *term_rec = rec;
  } else {
    /* destroy record */
    SNetRecDestroy( rec);
  }
}


/**
 * Restore the streams from the waitingset into
 * the readyset and send sort record if required
 */
static void RestoreFromWaitingset(
    snet_streamset_t *waitingset,
    snet_streamset_t *readyset,
    snet_record_t **sort_rec,
    snet_stream_desc_t *outstream)
{
  int level;
  assert( *sort_rec != NULL);
  /* waiting set contains all streams,
     ready set is empty => "swap" sets */
  *readyset = *waitingset;
  *waitingset = NULL;
  /* check sort record: if level > 0,
     decrement and forward to outstream */
  level = SNetRecGetLevel( *sort_rec);
  if( level > 0) {
    SNetRecSetLevel( *sort_rec, level-1);
    SNetStreamWrite( outstream, *sort_rec);
  } else {
    SNetRecDestroy( *sort_rec);
  }
  *sort_rec = NULL;
}


static void TerminateCollectorTask(snet_stream_desc_t *outstream,
                                   coll_arg_t *carg)
{
  if (carg->term_rec != NULL) {
    SNetRecDestroy(carg->term_rec);
  }
  if (carg->sort_rec != NULL) {
    SNetRecDestroy(carg->sort_rec);
  }
  /* close outstream */
  SNetStreamClose( outstream, false);

  SNetStreamListDestroy(carg->ready_list);
  SNetStreamListDestroy(carg->waiting_list);
  /* destroy argument */
  SNetMemFree( carg);
}

/*****************************************************************************/
/* COLLECTOR TASK                                                            */
/*****************************************************************************/

static void CollectorTask(snet_entity_t *ent, void *arg)
{
  snet_entity_t *newent;
  int index = 0;

  coll_arg_t *carg = (coll_arg_t *)arg;
  snet_streamset_t readyset, waitingset;
  snet_stream_desc_t *outstream, *curstream = NULL;
  snet_stream_iter_t *wait_iter;

  /* open outstream for writing */
  outstream = SNetStreamOpen(carg->output, 'w');
  if(carg->is_ready){
    index = carg->cur_index;
  }else{
    index = -1;
  }
  readyset = DequeueStreamList(carg->ready_list, &curstream, index);
  if(!carg->is_ready){
     index = carg->cur_index;
  }else{
    index = -1;
  }
  waitingset = DequeueStreamList(carg->waiting_list, &curstream, index);
  /* create an iterator for waiting set, is reused within main loop*/
  wait_iter = SNetStreamIterCreate( &waitingset);

  /* get a record */
  snet_record_t *rec = GetRecord(&readyset, carg->incount, &curstream);
  /* process the record */
  switch( SNetRecGetDescriptor( rec)) {

    case REC_data:
      /* data record: forward to output */
      SNetStreamWrite( outstream, rec);
      break;

    case REC_sort_end:
      ProcessSortRecord(ent, rec, &carg->sort_rec, curstream, &readyset, &waitingset);
      /* end processing this stream */
      curstream = NULL;
      break;

    case REC_sync:
      SNetStreamReplace( curstream, SNetRecGetStream( rec));
      SNetRecDestroy( rec);
      break;

    case REC_collect:
      /* only for dynamic collectors! */
      assert( false == carg->is_static );
      /* collect: add new stream to ready set */
#ifdef DESTROY_TERM_IN_WAITING_UPON_COLLECT
      /* but first, check if we can free resources by checking the
         waiting set for arrival of termination records */
      carg->incount -= DestroyTermInWaitingSet(wait_iter, &waitingset);
      assert(carg->incount > 0);
#endif
      /* finally, add new stream to ready set */
      SNetStreamsetPut(
          &readyset,
          SNetStreamOpen( SNetRecGetStream( rec), 'r')
          );
      /* update incoming counter */
      carg->incount++;
      /* destroy collect record */
      SNetRecDestroy( rec);
      break;

    case REC_terminate:
      /* termination record: close stream and remove from ready set */
      ProcessTermRecord(rec, &carg->term_rec, curstream, &readyset, &carg->incount);
      /* stop processing this stream */
      curstream = NULL;
      break;

    default:
      assert(0);
      /* if ignore, at least destroy ... */
      SNetRecDestroy( rec);
  } /* end switch */

  /************* termination conditions *****************/
  if ( SNetStreamsetIsEmpty( &readyset)) {
    /* the streams which had a sort record are in the waitingset */
    if ( !SNetStreamsetIsEmpty( &waitingset)) {
      /* check incount */
      if ( carg->is_static && (1 == carg->incount) ) {
        /* static collector has only one incoming stream left
         * -> this input stream can be sent to the collectors
         *    successor via a sync record
         * -> collector can terminate
         */
        snet_stream_desc_t *in = SNetStreamOpen(SNetStreamGet(waitingset), 'r');

        SNetStreamWrite( outstream,
            SNetRecCreate( REC_sync, SNetStreamGet(in))
            );
        SNetStreamClose( in, false);
#ifdef DEBUG_PRINT_GC
        /* terminating due to GC */
        SNetUtilDebugNoticeEnt( ent,
            "[COLL] Terminate static collector as only one branch left!"
            );
#endif
        TerminateCollectorTask(outstream,carg);
        SNetStreamIterDestroy(wait_iter);
        return;
      } else {
        RestoreFromWaitingset(&waitingset, &readyset, &carg->sort_rec, outstream);
      }
    } else {
      /* both ready set and waitingset are empty */
#ifdef DEBUG_PRINT_GC
      if (carg->is_static) {
        SNetUtilDebugNoticeEnt( ent,
            "[COLL] Terminate static collector as no inputs left!");
      }
#endif
      assert(carg->term_rec != NULL);
      SNetStreamWrite( outstream, carg->term_rec);
      carg->term_rec = NULL;
      TerminateCollectorTask(outstream,carg);
      SNetStreamIterDestroy(wait_iter);
      return;
    }
  }
  /************* end of termination conditions **********/
  index = FillList(carg->ready_list,&readyset,curstream);
  if(index == -1){
    carg->is_ready = false;
    carg->cur_index = -1;
    index = FillList(carg->waiting_list, &waitingset,curstream);
  }else{
    FillList(carg->waiting_list, &waitingset, NULL);
  }
  SNetStreamClose(outstream, false);

  SNetStreamIterDestroy(wait_iter);
  newent = SNetEntityCopy(ent);
  SNetThreadingSpawn(newent);
}

static void InitCollectorTask(snet_entity_t *ent, void *arg)
{
  snet_entity_t *newent;

  coll_arg_t *carg = (coll_arg_t *)arg;
  static int n = 0;
  int index = 0;
  snet_streamset_t readyset = NULL;
  n++;
  carg->n = n;

  //printf("%d: Init\n",carg->n);

  carg->cur = NULL;
  carg->sort_rec = NULL;
  carg->term_rec = NULL;


  if (carg->is_static) {
    int i;
    carg->incount = CARG_ST(carg, num);
    /* fill initial readyset of collector */
    for (i=0; i<carg->incount; i++) {
      snet_stream_desc_t *tmp;
      /* open each stream in listening set for reading */
      tmp = SNetStreamOpen( CARG_ST(carg, inputs[i]), 'r');
      /* add each stream instreams[i] to listening set of collector */
      SNetStreamsetPut( &readyset, tmp);
    }
    SNetMemFree( CARG_ST(carg, inputs) );
  } else {
    carg->incount = 1;
    /* Open initial stream and put into readyset */
    SNetStreamsetPut( &readyset,
        SNetStreamOpen(CARG_DYN(carg, input), 'r')
        );
  }
  carg->ready_list = SNetStreamListCreate(0);
  carg->waiting_list = SNetStreamListCreate(0);

  index = FillList(carg->ready_list,&readyset,NULL);
  if(index == -1){
    carg->is_ready = false;
    carg->cur_index = -1;
  }
  newent = SNetEntityCopy(ent);
  SNetEntitySetFunction(newent, &CollectorTask);
  SNetThreadingSpawn(newent);
}


/**
 * Collector task for dynamic combinators (star/split)
 * and the static parallel combinator
 */
void CollectorTaskBack(snet_entity_t *ent, void *arg)
{
  coll_arg_t *carg = (coll_arg_t *)arg;
  snet_streamset_t readyset, waitingset;
  snet_stream_iter_t *wait_iter;
  snet_stream_desc_t *outstream;
  snet_stream_desc_t *curstream = NULL;
  snet_record_t *sort_rec = NULL;
  snet_record_t *term_rec = NULL;
  int incount;
  bool terminate = false;


  /* open outstream for writing */
  outstream = SNetStreamOpen(carg->output, 'w');

  readyset = waitingset = NULL;

  if (carg->is_static) {
    int i;
    incount = CARG_ST(carg, num);
    /* fill initial readyset of collector */
    for (i=0; i<incount; i++) {
      snet_stream_desc_t *tmp;
      /* open each stream in listening set for reading */
      tmp = SNetStreamOpen( CARG_ST(carg, inputs[i]), 'r');
      /* add each stream instreams[i] to listening set of collector */
      SNetStreamsetPut( &readyset, tmp);
    }
    SNetMemFree( CARG_ST(carg, inputs) );
  } else {
    incount = 1;
    /* Open initial stream and put into readyset */
    SNetStreamsetPut( &readyset,
        SNetStreamOpen(CARG_DYN(carg, input), 'r')
        );
  }

  /* create an iterator for waiting set, is reused within main loop*/
  wait_iter = SNetStreamIterCreate( &waitingset);

  /* MAIN LOOP */
  while( !terminate) {
    /* get a record */
    snet_record_t *rec = GetRecord(&readyset, incount, &curstream);
    /* process the record */
    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        /* data record: forward to output */
        SNetStreamWrite( outstream, rec);
        break;

      case REC_sort_end:
        ProcessSortRecord(ent, rec, &sort_rec, curstream, &readyset, &waitingset);
        /* end processing this stream */
        curstream = NULL;
        break;

      case REC_sync:
        SNetStreamReplace( curstream, SNetRecGetStream( rec));
        SNetRecDestroy( rec);
        break;

      case REC_collect:
        /* only for dynamic collectors! */
        assert( false == carg->is_static );
        /* collect: add new stream to ready set */
#ifdef DESTROY_TERM_IN_WAITING_UPON_COLLECT
        /* but first, check if we can free resources by checking the
           waiting set for arrival of termination records */
        incount -= DestroyTermInWaitingSet(wait_iter, &waitingset);
        assert(incount > 0);
#endif
        /* finally, add new stream to ready set */
        SNetStreamsetPut(
            &readyset,
            SNetStreamOpen( SNetRecGetStream( rec), 'r')
            );
        /* update incoming counter */
        incount++;
        /* destroy collect record */
        SNetRecDestroy( rec);
        break;

      case REC_terminate:
        /* termination record: close stream and remove from ready set */
        ProcessTermRecord(rec, &term_rec, curstream, &readyset, &incount);
        /* stop processing this stream */
        curstream = NULL;
        break;

      default:
        assert(0);
        /* if ignore, at least destroy ... */
        SNetRecDestroy( rec);
    } /* end switch */

    /************* termination conditions *****************/
    if ( SNetStreamsetIsEmpty( &readyset)) {
      /* the streams which had a sort record are in the waitingset */
      if ( !SNetStreamsetIsEmpty( &waitingset)) {
        /* check incount */
        if ( carg->is_static && (1 == incount) ) {
          /* static collector has only one incoming stream left
           * -> this input stream can be sent to the collectors
           *    successor via a sync record
           * -> collector can terminate
           */
          snet_stream_desc_t *in = waitingset;

          SNetStreamWrite( outstream,
              SNetRecCreate( REC_sync, SNetStreamGet(in))
              );
          SNetStreamClose( in, false);
          terminate = true;
#ifdef DEBUG_PRINT_GC
          /* terminating due to GC */
          SNetUtilDebugNoticeEnt( ent,
              "[COLL] Terminate static collector as only one branch left!"
              );
#endif
        } else {
          RestoreFromWaitingset(&waitingset, &readyset, &sort_rec, outstream);
        }
      } else {
        /* both ready set and waitingset are empty */
#ifdef DEBUG_PRINT_GC
        if (carg->is_static) {
          SNetUtilDebugNoticeEnt( ent,
              "[COLL] Terminate static collector as no inputs left!");
        }
#endif
        assert(term_rec != NULL);
        SNetStreamWrite( outstream, term_rec);
        term_rec = NULL;
        terminate = true;
      }
    }
    /************* end of termination conditions **********/
  } /* MAIN LOOP END */

  if (term_rec != NULL) {
    SNetRecDestroy(term_rec);
  }
  if (sort_rec != NULL) {
    SNetRecDestroy(sort_rec);
  }
  /* close outstream */
  SNetStreamClose( outstream, false);
  /* destroy iterator */
  SNetStreamIterDestroy( wait_iter);
  /* destroy argument */
  SNetMemFree( carg);
} /* END of DYNAMIC COLLECTOR TASK */


/*****************************************************************************/
/* CREATION FUNCTIONS                                                        */
/*****************************************************************************/



/**
 * Collector creation function
 * @pre num >= 1
 */
snet_stream_t *CollectorCreateStatic( int num, snet_stream_t **instreams, int location, snet_info_t *info)
{
  snet_stream_t *outstream;
  coll_arg_t *carg;
  int i;

  assert(num >= 1);
  /* create outstream */
  outstream =  SNetStreamCreate(0);

  /* create collector handle */
  carg = (coll_arg_t *) SNetMemAlloc( sizeof( coll_arg_t));
  carg->output = outstream;
  carg->is_static = true;
  CARG_ST(carg, num) = num;
  /* copy instreams */
  CARG_ST(carg, inputs) = SNetMemAlloc(num * sizeof(snet_stream_t *));
  for(i=0; i<num; i++) {
    CARG_ST(carg, inputs[i]) = instreams[i];
  }

  /* spawn collector task */
  SNetThreadingSpawn(
      SNetEntityCreate( ENTITY_collect, location, SNetLocvecGet(info),
        "<collect>", &InitCollectorTask, (void*)carg)
      );
  return outstream;
}



/**
 * Collector creation function
 */
snet_stream_t *CollectorCreateDynamic( snet_stream_t *instream, int location, snet_info_t *info)
{
  snet_stream_t *outstream;
  coll_arg_t *carg;

  /* create outstream */
  outstream = SNetStreamCreate(0);

  /* create collector handle */
  carg = (coll_arg_t *) SNetMemAlloc( sizeof( coll_arg_t));
  carg->output = outstream;
  carg->is_static = false;
  CARG_DYN(carg, input) = instream;

  /* spawn collector task */
  SNetThreadingSpawn(
      SNetEntityCreate( ENTITY_collect, location, SNetLocvecGet(info),
        "<collect>", &InitCollectorTask, (void*)carg)
      );
  return outstream;
}

