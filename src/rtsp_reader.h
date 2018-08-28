#ifndef __RTSP_READER_DEF_H__
#define __RTSP_READER_DEF_H__

#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define RTSP_CONFIG_MAX_MSG_LENGTH          1024
#define RTSP_CONFIG_MAX_HEADERS             64

#define RTSP_TRUE   1
#define RTSP_FALSE  0

typedef struct
{
  uint8_t*    ptr;
  uint32_t    len;
} rtsp_str_t;

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

struct __rtsp_reader_t;
typedef struct __rtsp_reader_t rtsp_reader_t;

typedef int (*rtsp_state_handler)(rtsp_reader_t* rd, uint8_t c);

struct __rtsp_reader_t
{
  uint32_t              ndx;
  char*                 err_msg;

  rtsp_state_handler    main_state;
  rtsp_state_handler    sub_state;
  rtsp_state_handler    sub_eol_handler;

  rtsp_msg_t            current;

  //
  // temporary states for header parsing
  //
  uint8_t       colon_parsed;
  uint32_t      body_read;

  //
  // user callback
  //
  void (*rtsp_rx_cb)(rtsp_reader_t* rd, rtsp_msg_t* msg);
};

extern void rtsp_reader_init(rtsp_reader_t* rd, uint8_t request);
extern int rtsp_reader_handle_input(rtsp_reader_t* rd, uint8_t* buf, uint32_t len);

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

#endif /* !__RTSP_READER_DEF_H__ */
