#ifndef __RTSP_STR_DEF_H__
#define __RTSP_STR_DEF_H__

#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "rtsp_common.h"

typedef struct
{
  uint8_t*    ptr;
  uint32_t    len;
} rtsp_str_t;

static inline uint8_t
rtsp_str_cmp(rtsp_str_t* rtsp_str, const char* str)
{
  int l = strlen(str);

  if(l != rtsp_str->len)
  {
    return RTSP_FALSE;
  }

  if(strncmp((char*)rtsp_str->ptr, str, l) == 0)
  {
    return RTSP_TRUE;
  }

  return RTSP_FALSE;
}

static inline uint8_t
rtsp_str_casecmp(rtsp_str_t* rtsp_str, const char* str)
{
  int l = strlen(str);

  if(l != rtsp_str->len)
  {
    return RTSP_FALSE;
  }

  if(strncasecmp((char*)rtsp_str->ptr, str, l) == 0)
  {
    return RTSP_TRUE;
  }

  return RTSP_FALSE;
}

static inline uint64_t
rsp_str_to_u64(rtsp_str_t* rtsp_str)
{
  uint64_t    l = 0;
  uint32_t    mul = 1;

  for(uint32_t i = 0; i < rtsp_str->len; i++)
  {
    if(isdigit(rtsp_str->ptr[rtsp_str->len - i - 1]))
    {
      l += ((rtsp_str->ptr[rtsp_str->len - i - 1] - '0') * mul);
      mul *= 10;
    }
  }
  return l;
}

#endif /* !__RTSP_STR_DEF_H__ */
