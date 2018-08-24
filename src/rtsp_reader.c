#include <stdio.h>
#include "rtsp_reader.h"


//
// -1 : parsing state error
// -2 : push overflow error
//

#define RTSP_PUSH(rd, c)\
{\
  if(rd->ndx >= RTSP_CONFIG_MAX_MSG_LENGTH)\
  {\
    rd->err_msg = "buffer overflow";\
    return -2;\
  }\
  rd->msg[rd->ndx] = c;\
  rd->ndx++;\
}

#define RTSP_ERR(rd, ret, msg)\
{\
  rd->err_msg = msg;\
  return ret;\
}

////////////////////////////////////////////////////////////////////////////////
//
// RFC7826 parsing utilities
//
////////////////////////////////////////////////////////////////////////////////
static inline uint8_t
is_rfc7826_token(uint8_t c)
{
  if(c == 0x21 ||
     (c >= 0x23 && c <= 0x27) ||
     (c >= 0x2a && c <= 0x2b) ||
     (c >= 0x2d && c <= 0x2e) ||
     (c >= 0x30 && c <= 0x39) ||
     (c >= 0x41 && c <= 0x5a) ||
     (c >= 0x5e && c <= 0x7a) ||
     c == 0x7c ||
     c == 0x7e)
  {
    return RTSP_TRUE;
  }
  return RTSP_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
// parsing handlers for start line
//
////////////////////////////////////////////////////////////////////////////////
static inline int
rtsp_reader_start_line_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    rd->state = rtsp_reader_state_start_line_ignore_lr;
    return 0;
  }

  RTSP_PUSH(rd, c);
  rd->state = rtsp_reader_state_start_line_middle;

  return 0;
}

static inline int
rtsp_reader_start_line_ignore_lr(rtsp_reader_t* rd, uint8_t c)
{
  if(c != '\n')
  {
    RTSP_ERR(rd, -1, "NL expected at rtsp_reader_start_line_ignore_lr");
  }

  rd->state = rtsp_reader_state_start_line_begin;
  return 0;
}

static inline int
rtsp_reader_start_line_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    rd->state = rtsp_reader_state_start_line_end;
    return 0;
  }

  RTSP_PUSH(rd, c);
  return 0;
}

static inline int
rtsp_reader_start_line_end(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\n')
  {
    // FIXME check start line

    rd->state = rtsp_reader_state_header_line_begin;
    return 0;
  }

  RTSP_ERR(rd, -1, "NL expected at rtsp_reader_state_start_line_end");
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
//
// parsing handlers for headers
//
////////////////////////////////////////////////////////////////////////////////
static inline int
rtsp_reader_header_line_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    rd->state = rtsp_reader_state_headers_end;
    return 0;
  }

  RTSP_PUSH(rd, c);
  rd->state = rtsp_reader_state_header_line_middle;

  return 0;
}

static inline int
rtsp_reader_header_line_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    rd->state = rtsp_reader_state_header_line_end;
    return 0;
  }

  RTSP_PUSH(rd, c);
  return 0;
}

static inline int
rtsp_reader_header_line_end(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\n')
  {
    // FIXME check header line

    rd->state = rtsp_reader_state_header_line_begin;
    return 0;
  }

  RTSP_ERR(rd, -1, "NL expected at rtsp_reader_state_header_line_end");
}

static inline int
rtsp_reader_headers_end(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\n')
  {
    // FIXME check message integrity

    rd->state = rtsp_reader_state_body;
    return 0;
  }

  RTSP_ERR(rd, -1, "NL expected at rtsp_reader_state_headers_end");
}

////////////////////////////////////////////////////////////////////////////////
//
// parsing handlers for body
//
////////////////////////////////////////////////////////////////////////////////
static inline int
rtsp_reader_body(rtsp_reader_t* rd, uint8_t c)
{
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// publics
//
////////////////////////////////////////////////////////////////////////////////
void
rtsp_reader_init(rtsp_reader_t* rd)
{
  rd->ndx     = 0;
  rd->state   = rtsp_reader_state_start_line_begin;
  rd->err_msg = NULL;

  rd->body_len  = 0;
  rd->body_read = 0;
}

int
rtsp_reader_feed(rtsp_reader_t* rd, uint8_t* buf, uint32_t len)
{
  static const int (*_handlers[])(rtsp_reader_t* rd, uint8_t c) =
  {
    rtsp_reader_start_line_begin,
    rtsp_reader_start_line_ignore_lr,
    rtsp_reader_start_line_middle,
    rtsp_reader_start_line_end,

    rtsp_reader_header_line_begin,
    rtsp_reader_header_line_middle,
    rtsp_reader_header_line_end,
    rtsp_reader_headers_end,

    rtsp_reader_body,
  };

  for(uint32_t i = 0; i < len; i++)
  {
    if(_handlers[rd->state](rd, buf[i]) != 0)
    {
      return -1;
    }
  }
  return 0;
}
