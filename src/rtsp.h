#ifndef __RTSP_DEF_H__
#define __RTSP_DEF_H__

#include "rtsp_reader.h"

#define RTSP_EVENT_CONNECT          1
#define RTSP_EVENT_CLOSE            2
#define RTSP_EVENT_REQUEST          3
#define RTSP_EVENT_RESPONSE         4

typedef struct
{
} rtsp_connection_t;

typedef struct
{
} rtsp_mgt_t;

typedef void (*rtsp_event_handler)(rtsp_connection* conn, int ev, rtsp_msg_t* msg);

extern rtsp_mgr_init(rtsp_msg_t* mgr);
extern rtsp_mgr_deinit(rtsp_mgr_t* mgr);
extern rtsp_mgr_run(rtsp_msg_t* mgr);

extern rtsp_connection* rtsp_bind(int port, rtsp_event_handler handler);
extern rtsp_connection* rtsp_connect(rtsp_event_handler handler,
    const char* url, const char* hdrs, const char* body);

#endif /* !__RTSP_DEF_H__ */
