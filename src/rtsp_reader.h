#ifndef __RTSP_READER_DEF_H__
#define __RTSP_READER_DEF_H__

#include <stdint.h>

#define RTSP_CONFIG_MAX_MSG_LENGTH          1024

#define RTSP_TRUE   1
#define RTSP_FALSE  0

//
// XXX Note
//
// This implementation doesn't support
// a) SWS, that is, a header value that is span across multiple lines
// b) \r or \n escaped inside a quoted string
//

typedef enum
{
  rtsp_reader_state_idle,
  rtsp_reader_state_idle_cr,
  rtsp_reader_state_start_line_begin,
  rtsp_reader_state_start_line_middle,
  rtsp_reader_state_start_line_end,
  rtsp_reader_state_header_line_begin,
  rtsp_reader_state_header_line_middle,
  rtsp_reader_state_header_line_end,
  rtsp_reader_state_headers_end,
  rtsp_reader_state_body,
} rtsp_reader_state_t;

typedef struct
{
  uint8_t       msg[RTSP_CONFIG_MAX_MSG_LENGTH];
  uint32_t      ndx;
  char*         err_msg;

  uint32_t      body_len;
  uint32_t      body_read;

  rtsp_reader_state_t   state;
} rtsp_reader_t;

extern void rtsp_reader_init(rtsp_reader_t* rd);
extern int rtsp_reader_feed(rtsp_reader_t* rd, uint8_t* buf, uint32_t len);

#endif /* !__RTSP_READER_DEF_H__ */
