#include <stdio.h>
#include <stdlib.h>
#include "rtsp_reader.h"


/*
   XXX idea. Basically we have three weapons to deal with this beast.
   state machine, divide & conquer, and delegation!

   Personally, I think it's the dumbest idea to create a full blown parser that 
   we learned during compiler class at school for this beast.

   a) main_state only concerns itself with parsing line & LWS/SWS handling.
   b) detailed checks/translations/state machines  are handled in sub states.
   c) in reqeust/response line handling, special EOL event is triggered.
      In that handler, we can check validity of re quest/response start line
   d) that's all!

*/


#define RTSP_PUSH(rd, c)\
{\
  if(rd->ndx >= RTSP_CONFIG_MAX_MSG_LENGTH)\
  {\
    rd->err_msg = "buffer overflow";\
    return -2;\
  }\
  rd->current.msg[rd->ndx] = c;\
  rd->ndx++;\
}

#define RTSP_EXEC_SUB_STATE(rd, c)\
{\
  int __ret = rd->sub_state(rd, c);\
  if(__ret != 0)\
  {\
    return __ret;\
  }\
}

#define RTSP_EXEC_SUB_EOL(rd, c)\
{\
  int __ret = rd->sub_eol_handler(rd, c);\
  if(__ret != 0)\
  {\
    return __ret;\
  }\
}

#define RTSP_ERR(rd, ret, msg)\
{\
  rd->err_msg = msg;\
  return ret;\
}

////////////////////////////////////////////////////////////////////////////////
//
// utilities
//
////////////////////////////////////////////////////////////////////////////////

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
// request line parsing handlers
//
////////////////////////////////////////////////////////////////////////////////
static inline int rtsp_reader_req_line_state_method_begin(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_req_line_state_method_middle(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_req_line_state_uri_begin(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_req_line_state_uri_middle(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_req_line_state_ver_begin(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_req_line_state_ver_middle(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_req_line_eol_handler(rtsp_reader_t* rd, uint8_t c);

static inline int
rtsp_reader_req_line_state_method_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(is_rfc7826_token(c) == RTSP_TRUE)
  {
    rd->current.method.ptr = &rd->current.msg[rd->ndx];
    rd->current.method.len = 1;

    rd->sub_state = rtsp_reader_req_line_state_method_middle;
    RTSP_PUSH(rd, c);
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_req_line_state_method_begin");
}

static inline int
rtsp_reader_req_line_state_method_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ')
  {
    rd->sub_state = rtsp_reader_req_line_state_uri_begin;
    return 0;
  }

  if(is_rfc7826_token(c) == RTSP_TRUE)
  {
    rd->current.method.len++;
    RTSP_PUSH(rd, c);
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_req_line_state_method_middle");
}

static inline int
rtsp_reader_req_line_state_uri_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(is_rfc7826_token(c) == RTSP_TRUE)
  {
    rd->current.uri.ptr = &rd->current.msg[rd->ndx];
    rd->current.uri.len = 1;

    rd->sub_state = rtsp_reader_req_line_state_uri_middle;
    RTSP_PUSH(rd, c);
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_req_line_state_uri_begin");
}

static inline int
rtsp_reader_req_line_state_uri_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ')
  {
    rd->sub_state = rtsp_reader_req_line_state_ver_begin;
    return 0;
  }

  if(c == '\t')
  {
    RTSP_ERR(rd, -101, "HT at rtsp_reader_req_line_state_uri_middle");
  }

  rd->current.uri.len++;
  RTSP_PUSH(rd, c);
  return 0;
}

static inline int
rtsp_reader_req_line_state_ver_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(is_rfc7826_token(c) == RTSP_TRUE)
  {
    rd->current.ver.ptr = &rd->current.msg[rd->ndx];
    rd->current.ver.len = 1;

    rd->sub_state = rtsp_reader_req_line_state_ver_middle;
    RTSP_PUSH(rd, c);
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_req_line_state_ver_begin");
}

static inline int
rtsp_reader_req_line_state_ver_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ' || c == '\t')
  {
    RTSP_ERR(rd, -101, "SP or TAB in rtsp_reader_req_line_state_ver_middle");
    return -1;
  }

  rd->current.ver.len++;
  RTSP_PUSH(rd, c);
  return 0;
}

static inline int
rtsp_reader_req_line_eol_handler(rtsp_reader_t* rd, uint8_t c)
{
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// response line parsing handlers
//
////////////////////////////////////////////////////////////////////////////////
static inline int rtsp_reader_rsp_line_state_ver_begin(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_rsp_line_state_ver_middle(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_rsp_line_state_code_begin(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_rsp_line_state_code_middle(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_rsp_line_state_reason_begin(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_rsp_line_state_reason_middle(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_rsp_line_eol_handler(rtsp_reader_t* rd, uint8_t c);

static inline int
rtsp_reader_rsp_line_state_ver_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(is_rfc7826_token(c) == RTSP_TRUE)
  {
    rd->current.ver.ptr = &rd->current.msg[rd->ndx];
    rd->current.ver.len = 1;

    rd->sub_state = rtsp_reader_rsp_line_state_ver_middle;
    RTSP_PUSH(rd, c);
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_rsp_line_state_ver_begin");
}

static inline int
rtsp_reader_rsp_line_state_ver_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ')
  {
    rd->sub_state = rtsp_reader_rsp_line_state_code_begin;
    return 0;
  }

  rd->current.ver.len++;
  RTSP_PUSH(rd, c);
  return 0;
}

static inline int
rtsp_reader_rsp_line_state_code_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(isdigit(c))
  {
    rd->current.code.ptr = &rd->current.msg[rd->ndx];
    rd->current.code.len = 1;

    rd->sub_state = rtsp_reader_rsp_line_state_code_middle;
    RTSP_PUSH(rd, c);
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_rsp_line_state_code_begin");
}

static inline int
rtsp_reader_rsp_line_state_code_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ')
  {
    rd->sub_state = rtsp_reader_rsp_line_state_reason_begin;
    return 0;
  }

  if(isdigit(c))
  {
    rd->current.code.len++;
    RTSP_PUSH(rd, c);
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_rsp_line_state_code_middle");
}

static inline int
rtsp_reader_rsp_line_state_reason_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(is_rfc7826_token(c) == RTSP_TRUE)
  {
    rd->current.reason.ptr = &rd->current.msg[rd->ndx];
    rd->current.reason.len = 1;

    rd->sub_state = rtsp_reader_rsp_line_state_reason_middle;
    RTSP_PUSH(rd, c);
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_rsp_line_state_reason_begin");
}

static inline int
rtsp_reader_rsp_line_state_reason_middle(rtsp_reader_t* rd, uint8_t c)
{
  rd->current.reason.len++;
  RTSP_PUSH(rd, c);
  return 0;
}

static inline int
rtsp_reader_rsp_line_eol_handler(rtsp_reader_t* rd, uint8_t c)
{
  if(rd->current.code.len != 3)
  {
    RTSP_ERR(rd, -101, "code length is not zero");
  }

  if(rd->current.reason.len == 0)
  {
    RTSP_ERR(rd, -101, "empty reason phrase");
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// header line parsing handlers
//
////////////////////////////////////////////////////////////////////////////////
static inline int rtsp_reader_header_state_header_name_begin(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_header_state_header_name_middle(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_header_state_header_name_before_colon(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_header_state_header_name_after_colon(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_header_state_header_value(rtsp_reader_t* rd, uint8_t c);

static inline int
rtsp_reader_header_state_header_name_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ' || c == '\t')
  {
    RTSP_ERR(rd, -101, "SP or TAB in rtsp_reader_header_state_header_name_begin");
  }

  if(c == ':')
  {
    RTSP_ERR(rd, -101, "HCOLON in rtsp_reader_header_state_header_name_begin");
  }

  if(rd->current.num_headers >= RTSP_CONFIG_MAX_HEADERS)
  {
    RTSP_ERR(rd, -101, "headers overflow");
  }

  rd->current.headers[rd->current.num_headers].h.ptr = &rd->current.msg[rd->ndx];
  rd->current.headers[rd->current.num_headers].h.len = 1;

  rd->current.headers[rd->current.num_headers].v.len = 0;

  rd->sub_state = rtsp_reader_header_state_header_name_middle;
  RTSP_PUSH(rd, c);
  return 0;
}

static inline int
rtsp_reader_header_state_header_name_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ' || c == '\t')
  {
    rd->sub_state = rtsp_reader_header_state_header_name_before_colon;
    return 0;
  }

  if(c == ':')
  {
    rd->colon_parsed = RTSP_TRUE;
    rd->sub_state = rtsp_reader_header_state_header_name_after_colon;
    return 0;
  }

  rd->current.headers[rd->current.num_headers].h.len++;
  RTSP_PUSH(rd, c);
  return 0;
}

static inline int
rtsp_reader_header_state_header_name_before_colon(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ' || c == '\t')
  {
    return 0;
  }

  if(c == ':')
  {
    rd->colon_parsed = RTSP_TRUE;
    rd->sub_state = rtsp_reader_header_state_header_name_after_colon;
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_header_state_header_name_before_colon");
}

static inline int
rtsp_reader_header_state_header_name_after_colon(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ' || c == '\t')
  {
    return 0;
  }

  if(c == ':')
  {
    RTSP_ERR(rd, -101, "HCOLON in rtsp_reader_header_state_header_name_after_colon");
  }

  rd->current.headers[rd->current.num_headers].v.ptr = &rd->current.msg[rd->ndx];
  rd->current.headers[rd->current.num_headers].v.len = 1;

  rd->sub_state = rtsp_reader_header_state_header_value;
  RTSP_PUSH(rd, c);
  return 0;
}

static inline int
rtsp_reader_header_state_header_value(rtsp_reader_t* rd, uint8_t c)
{
  rd->current.headers[rd->current.num_headers].v.len++;
  RTSP_PUSH(rd, c);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// main parsing handlers 
//
////////////////////////////////////////////////////////////////////////////////
static inline int rtsp_reader_start_line_begin(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_start_line_ignore_lr(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_start_line_middle(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_start_line_end(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_header_line_begin(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_header_line_begin_or_sws(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_header_line_sws(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_header_line_middle(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_header_line_end(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_headers_end(rtsp_reader_t* rd, uint8_t c);
static inline int rtsp_reader_body(rtsp_reader_t* rd, uint8_t c);

static inline int
rtsp_reader_start_line_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    rd->main_state = rtsp_reader_start_line_ignore_lr;
    return 0;
  }

  RTSP_EXEC_SUB_STATE(rd, c);
  rd->main_state = rtsp_reader_start_line_middle;

  return 0;
}

static inline int
rtsp_reader_start_line_ignore_lr(rtsp_reader_t* rd, uint8_t c)
{
  if(c != '\n')
  {
    RTSP_ERR(rd, -1, "NL expected at rtsp_reader_start_line_ignore_lr");
  }

  rd->main_state = rtsp_reader_start_line_begin;
  return 0;
}

static inline int
rtsp_reader_start_line_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    rd->main_state = rtsp_reader_start_line_end;
    return 0;
  }

  RTSP_EXEC_SUB_STATE(rd, c);

  return 0;
}

static inline int
rtsp_reader_start_line_end(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\n')
  {
    RTSP_EXEC_SUB_EOL(rd, c);

    rd->main_state = rtsp_reader_header_line_begin;
    return 0;
  }

  RTSP_ERR(rd, -1, "NL expected at rtsp_reader_state_start_line_end");
  return -1;
}

static inline int
rtsp_reader_header_line_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    rd->main_state = rtsp_reader_headers_end;
    return 0;
  }

  rd->colon_parsed = RTSP_FALSE;

  rd->sub_state = rtsp_reader_header_state_header_name_begin;
  RTSP_EXEC_SUB_STATE(rd, c);

  rd->main_state = rtsp_reader_header_line_middle;

  return 0;
}

static inline int
rtsp_reader_header_line_begin_or_sws(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    //
    // header fields are finished
    //
    rd->current.num_headers++;

    rd->main_state = rtsp_reader_headers_end;
    return 0;
  }

  if(c == ' ' || c == '\t')
  {
    // sws continuation
    if(rd->colon_parsed == RTSP_FALSE)
    {
      RTSP_ERR(rd, -1, "Line Continuation Before Colon Detected");
    }

    rd->main_state = rtsp_reader_header_line_sws;
    return 0;
  }

  //
  // previous header line end
  //
  rd->current.num_headers++;

  //
  // header line begin
  //
  rd->colon_parsed = RTSP_FALSE;
  rd->sub_state = rtsp_reader_header_state_header_name_begin;
  RTSP_EXEC_SUB_STATE(rd, c);

  rd->main_state = rtsp_reader_header_line_middle;

  return 0;
}

static inline int
rtsp_reader_header_line_sws(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ' || c == '\t')
  {
    // continue
    return 0;
  }

  //
  // line continuation end
  // LWS is replaced with single SP
  //
  RTSP_EXEC_SUB_STATE(rd, ' ');

  if(c == '\r')
  {
    // another end possibility
    rd->main_state = rtsp_reader_header_line_end;
    return 0;
  }

  RTSP_EXEC_SUB_STATE(rd, c);
  rd->main_state = rtsp_reader_header_line_middle;
  return 0;
}

static inline int
rtsp_reader_header_line_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    rd->main_state = rtsp_reader_header_line_end;
    return 0;
  }

  RTSP_EXEC_SUB_STATE(rd, c);
  return 0;
}

static inline int
rtsp_reader_header_line_end(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\n')
  {
    rd->main_state = rtsp_reader_header_line_begin_or_sws;
    return 0;
  }

  RTSP_ERR(rd, -1, "NL expected at rtsp_reader_state_header_line_end");
}

static inline int
rtsp_reader_headers_end(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\n')
  {
    rtsp_hv_t*    cl;

    cl = rtsp_msg_get_header(&rd->current, "Content-Length");
    if(cl != NULL)
    {
      rd->current.body.len = rsp_str_to_u64(&cl->v);
    }

    if(rd->current.body.len != 0)
    {
      rd->sub_state  = NULL;
      rd->main_state = rtsp_reader_body;
    }
    else
    {
      rd->sub_state  = NULL;
      rd->main_state = NULL;

      rd->current.msg_len = rd->ndx;

      rd->rtsp_rx_cb(rd, &rd->current);
    }
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
  RTSP_PUSH(rd, c);
  rd->body_read++;

  if(rd->body_read >= rd->current.body.len)
  {
    rd->current.msg_len = rd->ndx;

    rd->rtsp_rx_cb(rd, &rd->current);
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// publics
//
////////////////////////////////////////////////////////////////////////////////
void
rtsp_reader_init(rtsp_reader_t* rd, uint8_t req)
{
  rd->ndx           = 0;

  rd->main_state      = rtsp_reader_start_line_begin;

  if(req == RTSP_TRUE)
  {
    rd->sub_state       = rtsp_reader_req_line_state_method_begin;
    rd->sub_eol_handler = rtsp_reader_req_line_eol_handler;
  }
  else
  {
    rd->sub_state       = rtsp_reader_rsp_line_state_ver_begin;
    rd->sub_eol_handler = rtsp_reader_rsp_line_eol_handler;
  }

  rd->err_msg       = NULL;

  memset(&rd->current, 0, sizeof(rtsp_msg_t));

  rd->body_read     = 0;
}

int
rtsp_reader_handle_input(rtsp_reader_t* rd, uint8_t* buf, uint32_t len)
{
  for(uint32_t i = 0; i < len; i++)
  {
    if(rd->main_state(rd, buf[i]) != 0)
    {
      printf("ERR: %s\n", rd->err_msg);
      return -1;
    }
  }
  return 0;
}
