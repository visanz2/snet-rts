#ifndef _COLLECTORS_H_
#define _COLLECTORS_H_

#include "snettypes.h"
#include "info.h"


snet_stream_t *CollectorCreateStatic( int num, snet_stream_t **instreams, snet_info_t *info);


snet_stream_t *CollectorCreateDynamic( snet_stream_t *instream, snet_info_t *info);

#endif
