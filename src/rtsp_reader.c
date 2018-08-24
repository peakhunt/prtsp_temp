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

#define RTSP_EXEC_SUB_SATTE(rd, c)\
{\
  int __ret = rd->sub_state(rd, c);\
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
static int rtsp_reader_req_line_state_method_begin(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_req_line_state_method_middle(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_req_line_state_uri_begin(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_req_line_state_uri_middle(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_req_line_state_ver_begin(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_req_line_state_ver_middle(rtsp_reader_t* rd, uint8_t c);

static int
rtsp_reader_req_line_state_method_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(is_rfc7826_token(c) == RTSP_TRUE)
  {
    rd->method.ptr = &rd->msg[rd->ndx];
    rd->method.len = 1;

    rd->sub_state = rtsp_reader_req_line_state_method_middle;
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_req_line_state_method_begin");
}

static int
rtsp_reader_req_line_state_method_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ' || c == '\t')
  {
    rd->sub_state = rtsp_reader_req_line_state_uri_begin;
    return 0;
  }

  if(is_rfc7826_token(c) == RTSP_TRUE)
  {
    rd->method.len++;
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_req_line_state_method_middle");
}

static int
rtsp_reader_req_line_state_uri_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(is_rfc7826_token(c) == RTSP_TRUE)
  {
    rd->uri.ptr = &rd->msg[rd->ndx];
    rd->uri.len = 1;

    rd->sub_state = rtsp_reader_req_line_state_uri_middle;
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_req_line_state_uri_begin");
}

static int
rtsp_reader_req_line_state_uri_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ' || c == '\t')
  {
    rd->sub_state = rtsp_reader_req_line_state_ver_begin;
    return 0;
  }

  rd->uri.len++;
  return 0;
}

static int
rtsp_reader_req_line_state_ver_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(is_rfc7826_token(c) == RTSP_TRUE)
  {
    rd->ver.ptr = &rd->msg[rd->ndx];
    rd->ver.len = 1;

    rd->sub_state = rtsp_reader_req_line_state_ver_middle;
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_req_line_state_ver_begin");
}

static int
rtsp_reader_req_line_state_ver_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ' || c == '\t')
  {
    RTSP_ERR(rd, -101, "SP or TAB in rtsp_reader_req_line_state_ver_middle");
    return -1;
  }

  rd->ver.len++;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// header line parsing handlers
//
////////////////////////////////////////////////////////////////////////////////
static int rtsp_reader_header_state_header_name_begin(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_header_state_header_name_middle(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_header_state_header_name_before_colon(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_header_state_header_name_after_colon(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_header_state_header_value(rtsp_reader_t* rd, uint8_t c);

static int
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

  if(rd->num_headers >= RTSP_CONFIG_MAX_HEADERS)
  {
    RTSP_ERR(rd, -101, "headers overflow");
  }

  rd->headers[rd->num_headers].h.ptr = &rd->msg[rd->ndx];
  rd->headers[rd->num_headers].h.len = 1;

  rd->headers[rd->num_headers].v.len = 0;

  rd->sub_state = rtsp_reader_header_state_header_name_middle;
  return 0;
}

static int
rtsp_reader_header_state_header_name_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ' || c == '\t')
  {
    rd->sub_state = rtsp_reader_header_state_header_name_before_colon;
    return 0;
  }

  if(c == ':')
  {
    rd->sub_state = rtsp_reader_header_state_header_name_after_colon;
    return 0;
  }

  rd->headers[rd->num_headers].h.len++;
  return 0;
}

static int
rtsp_reader_header_state_header_name_before_colon(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ' || c == '\t')
  {
    return 0;
  }

  if(c == ':')
  {
    rd->sub_state = rtsp_reader_header_state_header_name_after_colon;
    return 0;
  }

  RTSP_ERR(rd, -101, "invalid character in rtsp_reader_header_state_header_name_before_colon");
}

static int
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

  rd->headers[rd->num_headers].v.ptr = &rd->msg[rd->ndx];
  rd->headers[rd->num_headers].v.len = 1;

  rd->sub_state = rtsp_reader_header_state_header_value;
  return 0;
}

static int
rtsp_reader_header_state_header_value(rtsp_reader_t* rd, uint8_t c)
{
  rd->headers[rd->num_headers].v.len++;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// main parsing handlers 
//
////////////////////////////////////////////////////////////////////////////////
static int rtsp_reader_start_line_begin(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_start_line_ignore_lr(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_start_line_middle(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_start_line_end(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_header_line_begin(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_header_line_begin_or_sws(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_header_line_sws(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_header_line_middle(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_header_line_end(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_headers_end(rtsp_reader_t* rd, uint8_t c);
static int rtsp_reader_body(rtsp_reader_t* rd, uint8_t c);

static int
rtsp_reader_start_line_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    rd->main_state = rtsp_reader_start_line_ignore_lr;
    return 0;
  }

  RTSP_EXEC_SUB_SATTE(rd, c);
  RTSP_PUSH(rd, c);

  rd->main_state = rtsp_reader_start_line_middle;

  return 0;
}

static int
rtsp_reader_start_line_ignore_lr(rtsp_reader_t* rd, uint8_t c)
{
  if(c != '\n')
  {
    RTSP_ERR(rd, -1, "NL expected at rtsp_reader_start_line_ignore_lr");
  }

  rd->main_state = rtsp_reader_start_line_begin;
  return 0;
}

static int
rtsp_reader_start_line_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    rd->main_state = rtsp_reader_start_line_end;
    return 0;
  }

  RTSP_EXEC_SUB_SATTE(rd, c);
  RTSP_PUSH(rd, c);

  return 0;
}

static int
rtsp_reader_start_line_end(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\n')
  {
    rd->main_state = rtsp_reader_header_line_begin;
    return 0;
  }

  RTSP_ERR(rd, -1, "NL expected at rtsp_reader_state_start_line_end");
  return -1;
}

static int
rtsp_reader_header_line_begin(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    rd->main_state = rtsp_reader_headers_end;
    return 0;
  }

  rd->sub_state = rtsp_reader_header_state_header_name_begin;
  RTSP_EXEC_SUB_SATTE(rd, c);

  RTSP_PUSH(rd, c);
  rd->main_state = rtsp_reader_header_line_middle;

  return 0;
}

static int
rtsp_reader_header_line_begin_or_sws(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    //
    // header fields are finished
    //
    rd->num_headers++;

    rd->main_state = rtsp_reader_headers_end;
    return 0;
  }

  if(c == ' ' || c == '\t')
  {
    // sws continuation
    rd->main_state = rtsp_reader_header_line_sws;
    return 0;
  }

  //
  // previous header line end
  //
  rd->num_headers++;

  //
  // header line begin
  //
  rd->sub_state = rtsp_reader_header_state_header_name_begin;
  RTSP_EXEC_SUB_SATTE(rd, c);

  RTSP_PUSH(rd, c);
  rd->main_state = rtsp_reader_header_line_middle;

  return 0;
}

static int
rtsp_reader_header_line_sws(rtsp_reader_t* rd, uint8_t c)
{
  if(c == ' ' || c == '\t')
  {
    // continue
    return 0;
  }

  if(c == '\r')
  {
    // another end possibility
    rd->main_state = rtsp_reader_header_line_end;
    return 0;
  }

  // line continuation end
  RTSP_EXEC_SUB_SATTE(rd, ' ');
  RTSP_PUSH(rd, ' ');

  RTSP_EXEC_SUB_SATTE(rd, c);
  RTSP_PUSH(rd, c);

  rd->main_state = rtsp_reader_header_line_middle;
  return 0;
}

static int
rtsp_reader_header_line_middle(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\r')
  {
    rd->main_state = rtsp_reader_header_line_end;
    return 0;
  }

  RTSP_EXEC_SUB_SATTE(rd, c);
  RTSP_PUSH(rd, c);
  return 0;
}

static int
rtsp_reader_header_line_end(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\n')
  {
    rd->main_state = rtsp_reader_header_line_begin_or_sws;
    return 0;
  }

  RTSP_ERR(rd, -1, "NL expected at rtsp_reader_state_header_line_end");
}

static int
rtsp_reader_headers_end(rtsp_reader_t* rd, uint8_t c)
{
  if(c == '\n')
  {
    // FIXME check message integrity

    rd->sub_state  = NULL;
    rd->main_state = rtsp_reader_body;
    return 0;
  }

  RTSP_ERR(rd, -1, "NL expected at rtsp_reader_state_headers_end");
}

////////////////////////////////////////////////////////////////////////////////
//
// parsing handlers for body
//
////////////////////////////////////////////////////////////////////////////////
static int
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
  rd->ndx           = 0;

  rd->main_state      = rtsp_reader_start_line_begin;
  rd->sub_state       = rtsp_reader_req_line_state_method_begin;

  rd->err_msg       = NULL;

  rd->body_len      = 0;
  rd->body_read     = 0;

  rd->num_headers   = 0;
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
