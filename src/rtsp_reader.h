#ifndef __RTSP_READER_DEF_H__
#define __RTSP_READER_DEF_H__

#include <stdint.h>
#include <string.h>

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

struct __rtsp_reader_t;
typedef struct __rtsp_reader_t rtsp_reader_t;

typedef int (*rtsp_state_handler)(rtsp_reader_t* rd, uint8_t c);

struct __rtsp_reader_t
{
  uint8_t       msg[RTSP_CONFIG_MAX_MSG_LENGTH];
  uint32_t      ndx;
  char*         err_msg;

  uint32_t      body_len;
  uint32_t      body_read;

  rtsp_state_handler    main_state;
  rtsp_state_handler    sub_state;

  rtsp_str_t    method;
  rtsp_str_t    uri;

  rtsp_str_t    code;
  rtsp_str_t    reason;

  rtsp_str_t    ver;

  uint32_t      num_headers;
  rtsp_hv_t     headers[RTSP_CONFIG_MAX_HEADERS];

  //
  // temporary states for header parsing
  //
  uint8_t       colon_parsed;
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

#endif /* !__RTSP_READER_DEF_H__ */
