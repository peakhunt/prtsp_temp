#include "CUnit/Basic.h"
#include "CUnit/Console.h"
#include "CUnit/Automated.h"

#include <stdlib.h>
#include <stdio.h>

#include "rtsp_reader.h"

int init_suite_success(void) { return 0; }
int init_suite_failure(void) { return -1; }
int clean_suite_success(void) { return 0; }
int clean_suite_failure(void) { return -1; }

static void
test_rtsp_reader1(void)
{
  rtsp_reader_t   reader;
  int             ret;
  static const char* test_msg = \
  "\r\n" \
  "\r\n" \
  "PLAY rtsp://test.rtsp.com RTSP/2.0\r\n"\
  "header1: header1-value\r\n" \
  "header2 :\t header2-value\r\n" \
  "header3    :      header3-value\r\n" \
  "header4       :\theader4-value\r\n" \
  " \t             -value\r\n" \
  "header5\t:\t\t\t header5          -value          \r\n" \
  "\r\n";

  rtsp_reader_init(&reader);

  ret = rtsp_reader_handle_input(&reader, (uint8_t*)test_msg, strlen(test_msg));
  CU_ASSERT(ret == 0);

  CU_ASSERT(rtsp_str_cmp(&reader.method, "PLAY") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.uri, "rtsp://test.rtsp.com") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.ver, "RTSP/2.0") == RTSP_TRUE);

  CU_ASSERT(reader.num_headers == 5);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[0].h, "header1") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[1].h, "header2") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[2].h, "header3") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[3].h, "header4") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[4].h, "header5") == RTSP_TRUE);

  CU_ASSERT(rtsp_str_cmp(&reader.headers[0].v, "header1-value") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[1].v, "header2-value") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[2].v, "header3-value") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[3].v, "header4-value -value") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[4].v, "header5          -value          ") == RTSP_TRUE);
}

static void
test_rtsp_reader2(void)
{
  rtsp_reader_t   reader;
  int             ret;
  static const char* test_msg1 = \
  "PLAY rtsp://test.rtsp.com ";

  static const char* test_msg2 = \
  "RTSP/2.0\r\n"\
  "header1: header1-value\r\n";

  static const char* test_msg3 = \
  "header2: header2-value\r\n" \
  "header3: header3-value\r\n" \
  "\r\n";

  rtsp_reader_init(&reader);

  ret = rtsp_reader_handle_input(&reader, (uint8_t*)test_msg1, strlen(test_msg1));
  CU_ASSERT(ret == 0);

  ret = rtsp_reader_handle_input(&reader, (uint8_t*)test_msg2, strlen(test_msg2));
  CU_ASSERT(ret == 0);

  ret = rtsp_reader_handle_input(&reader, (uint8_t*)test_msg3, strlen(test_msg3));
  CU_ASSERT(ret == 0);

  CU_ASSERT(rtsp_str_cmp(&reader.method, "PLAY") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.uri, "rtsp://test.rtsp.com") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.ver, "RTSP/2.0") == RTSP_TRUE);

  CU_ASSERT(reader.num_headers == 3);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[0].h, "header1") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[1].h, "header2") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[2].h, "header3") == RTSP_TRUE);

  CU_ASSERT(rtsp_str_cmp(&reader.headers[0].v, "header1-value") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[1].v, "header2-value") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[2].v, "header3-value") == RTSP_TRUE);
}

static void
test_rtsp_reader3(void)
{
  rtsp_reader_t   reader;
  int             ret;
  static const char* test_msg1 = \
  "ANNOUNCE rtsp://example.com/media.mp4 RTSP/2.0\r\n" \
  "CSeq: 7\r\n" \
  "Date: 23 Jan 1997 15:35:06 GMT\r\n" \
  "Session: 12345678\r\n" \
  "Content-Type: application/sdp\r\n" \
  "Content-Length: 332\r\n" \
  "Zolla:\r\n" \
  " Cool\r\n" \
  " Lala\r\n" \
  "\r\n";

  rtsp_reader_init(&reader);

  ret = rtsp_reader_handle_input(&reader, (uint8_t*)test_msg1, strlen(test_msg1));
  CU_ASSERT(ret == 0);

  CU_ASSERT(rtsp_str_cmp(&reader.method, "ANNOUNCE") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.uri, "rtsp://example.com/media.mp4") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.ver, "RTSP/2.0") == RTSP_TRUE);

  CU_ASSERT(reader.num_headers == 6);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[0].h, "CSeq") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[1].h, "Date") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[2].h, "Session") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[3].h, "Content-Type") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[4].h, "Content-Length") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[5].h, "Zolla") == RTSP_TRUE);

  CU_ASSERT(rtsp_str_cmp(&reader.headers[0].v, "7") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[1].v, "23 Jan 1997 15:35:06 GMT") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[2].v, "12345678") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[3].v, "application/sdp") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[4].v, "332") == RTSP_TRUE);
  CU_ASSERT(rtsp_str_cmp(&reader.headers[5].v, "Cool Lala") == RTSP_TRUE);
}

static void test_rtsp_fail(void)
{
  rtsp_reader_t   reader;
  int             ret;
  static const char* test_msg1 = \
  "PLAY rtsp://test.rtsp.com ";

  static const char* test_msg2 = \
  "RTSP/2.0\r\n"\
  "header1: header1-value\r\n";

  static const char* test_msg3 = \
  "header2: header2-value\r\n" \
  "header3: header3-value\r\n" \
  "abnormal-but-accepted   \r\n" \
  " : this is ok with this implementation\r\n" \
  "\r\n";

  rtsp_reader_init(&reader);

  ret = rtsp_reader_handle_input(&reader, (uint8_t*)test_msg1, strlen(test_msg1));
  CU_ASSERT(ret == 0);

  ret = rtsp_reader_handle_input(&reader, (uint8_t*)test_msg2, strlen(test_msg2));
  CU_ASSERT(ret == 0);

  ret = rtsp_reader_handle_input(&reader, (uint8_t*)test_msg3, strlen(test_msg3));
  CU_ASSERT(ret != 0);
}

int
main()
{
  CU_pSuite pSuite = NULL;

  /* initialize the CUnit test registry */
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  /* add a suite to the registry */
  pSuite = CU_add_suite("Suite_success", init_suite_success, clean_suite_success);

  if (NULL == pSuite) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  CU_add_test(pSuite, "test_rtsp_reader1", test_rtsp_reader1);
  CU_add_test(pSuite, "test_rtsp_reader2", test_rtsp_reader2);
  CU_add_test(pSuite, "test_rtsp_reader3", test_rtsp_reader3);
  CU_add_test(pSuite, "test_rtsp_fail", test_rtsp_fail);

  /* Run all tests using the basic interface */
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  printf("\n");
  CU_basic_show_failures(CU_get_failure_list());
  printf("\n\n");

  /* Clean up registry and return */
  CU_cleanup_registry();
  return CU_get_error();
}
