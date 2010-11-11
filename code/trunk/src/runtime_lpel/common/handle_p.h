#ifndef _HANDLE_P_H_
#define _HANDLE_P_H_

#include "handle.h"
#include "stream.h"

typedef struct {
  int num;
  char **string_names;
  int *int_names;
} name_mapping_t;


struct handle {
  snet_record_t *rec;
  snet_box_sign_t *sign;
  name_mapping_t *mapping;
  stream_desc_t *out_sd;
};

void SNetHndDestroy( snet_handle_t *hnd);

#endif /* _HANDLE_P_H_ */
