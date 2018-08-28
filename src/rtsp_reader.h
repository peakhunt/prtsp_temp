#ifndef __RTSP_READER_DEF_H__
#define __RTSP_READER_DEF_H__

#include "rtsp_common.h"
#include "rtsp_str.h"
#include "rtsp_msg.h"

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

#endif /* !__RTSP_READER_DEF_H__ */
