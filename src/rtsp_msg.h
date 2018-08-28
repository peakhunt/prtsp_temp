#ifndef __RTSP_MSG_DEF_H__
#define __RTSP_MSG_DEF_H__

#include <stdint.h>
#include "rtsp_str.h"

#define RTSP_CONFIG_MAX_MSG_LENGTH          1024
#define RTSP_CONFIG_MAX_HEADERS             64

typedef struct
{
  rtsp_str_t    h;
  rtsp_str_t    v;
} rtsp_hv_t;

typedef struct
{
  uint8_t               msg[RTSP_CONFIG_MAX_MSG_LENGTH];
  uint32_t              msg_len;

  rtsp_str_t    method;
  rtsp_str_t    uri;
  rtsp_str_t    code;
  rtsp_str_t    reason;
  rtsp_str_t    ver;

  uint32_t      num_headers;
  rtsp_hv_t     headers[RTSP_CONFIG_MAX_HEADERS];

  rtsp_str_t    body;

  uint8_t       req_rsp;
} rtsp_msg_t;

static inline rtsp_hv_t*
rtsp_msg_get_header(rtsp_msg_t* msg, const char* hn)
{
  for(uint32_t i = 0; i < msg->num_headers; i++)
  {
    if(rtsp_str_casecmp(&msg->headers[i].h, hn) == RTSP_TRUE)
    {
      return &msg->headers[i];
    }
  }
  return NULL;
}

static inline void
rtsp_msg_deep_copy(rtsp_msg_t* dest, rtsp_msg_t* src)
{
  memcpy(dest, src, sizeof(rtsp_msg_t));

  dest->method.ptr = src->method.ptr == NULL ? NULL : &dest->msg[src->method.ptr - src->msg];
  dest->uri.ptr = src->uri.ptr == NULL ? NULL : &dest->msg[src->uri.ptr - src->msg];
  dest->code.ptr = src->code.ptr == NULL ? NULL : &dest->msg[src->code.ptr - src->msg];
  dest->reason.ptr = src->reason.ptr == NULL ? NULL : &dest->msg[src->reason.ptr - src->msg];
  dest->ver.ptr = src->ver.ptr == NULL ? NULL : &dest->msg[src->ver.ptr - src->msg];

  for(uint32_t i = 0; i < src->num_headers; i++)
  {
    dest->headers[i].h.ptr = &dest->msg[src->headers[i].h.ptr - src->msg];
    dest->headers[i].v.ptr = &dest->msg[src->headers[i].v.ptr - src->msg];
  }
}

#endif /* !__RTSP_MSG_DEF_H__ */
