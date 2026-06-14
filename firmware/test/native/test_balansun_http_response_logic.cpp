#include <gtest/gtest.h>

#include "balansun_http_response_logic.h"

TEST(BalansunHttpResponseLogic, StatusOkFor2xx) {
  EXPECT_TRUE(balansun_http_response_status_ok("HTTP/1.0 200 OK\r\n"));
  EXPECT_TRUE(balansun_http_response_status_ok("HTTP/1.1 204 No Content\r\n"));
  EXPECT_FALSE(balansun_http_response_status_ok("HTTP/1.1 301 Moved\r\n"));
  EXPECT_FALSE(balansun_http_response_status_ok("not http"));
}

TEST(BalansunHttpResponseLogic, ExtractHttp10JsonBody) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/json\r\n"
      "Connection: close\r\n"
      "\r\n"
      R"({"dateJour":"2026-05-21","codeJour":1,"libCouleur":"Bleu"})";
  const std::string body = balansun_http_response_extract_body(wire);
  EXPECT_EQ(body, R"({"dateJour":"2026-05-21","codeJour":1,"libCouleur":"Bleu"})");
}

TEST(BalansunHttpResponseLogic, ExtractChunkedBody) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "26\r\n"
      R"({"dateJour":"2026-05-21","codeJour":1})"
      "\r\n"
      "0\r\n"
      "\r\n";
  const std::string body = balansun_http_response_extract_body(wire);
  EXPECT_EQ(body, R"({"dateJour":"2026-05-21","codeJour":1})");
}

TEST(BalansunHttpResponseLogic, RejectsRedirectAndEmpty) {
  const std::string redirect =
      "HTTP/1.1 301 Moved\r\nLocation: https://example/\r\n\r\n\r\n";
  EXPECT_TRUE(balansun_http_response_extract_body(redirect).empty());
  const std::string empty_ok = "HTTP/1.0 200 OK\r\n\r\n";
  EXPECT_TRUE(balansun_http_response_extract_body(empty_ok).empty());
}

TEST(BalansunHttpResponseLogic, StatusRejectsMalformedStatusLine) {
  EXPECT_FALSE(balansun_http_response_status_ok("HTTP/1.1"));
  EXPECT_FALSE(balansun_http_response_status_ok("HTTP/1.1 20a OK\r\n"));
  EXPECT_FALSE(balansun_http_response_status_ok("HTTP/1.1 199 OK\r\n"));
  EXPECT_FALSE(balansun_http_response_status_ok("HTTP/1.1 300 OK\r\n"));
}

TEST(BalansunHttpResponseLogic, ExtractRejectsMissingHeaderTerminator) {
  EXPECT_TRUE(balansun_http_response_extract_body("HTTP/1.1 200 OK\r\nContent-Type: json").empty());
}

TEST(BalansunHttpResponseLogic, ExtractTrimsTrailingWhitespaceOnIdentityBody) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/json\r\n"
      "\r\n"
      "{\"ok\":true}  \r\n\n";
  EXPECT_EQ(balansun_http_response_extract_body(wire), "{\"ok\":true}");
}

TEST(BalansunHttpResponseLogic, ChunkedHeaderCaseInsensitiveWithTab) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "content-type: text/plain\r\n"
      "transfer-encoding:\tchunked\r\n"
      "\r\n"
      "5\r\n"
      "hello"
      "\r\n"
      "0\r\n"
      "\r\n";
  EXPECT_EQ(balansun_http_response_extract_body(wire), "hello");
}

TEST(BalansunHttpResponseLogic, ChunkedSkipsUnrelatedHeaderLines) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "X-Transfer-Encoding: identity\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "3\r\n"
      "abc"
      "\r\n"
      "0\r\n"
      "\r\n";
  EXPECT_EQ(balansun_http_response_extract_body(wire), "abc");
}

TEST(BalansunHttpResponseLogic, ChunkedRejectsInvalidSizeLine) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "GG\r\n"
      "0\r\n"
      "\r\n";
  EXPECT_TRUE(balansun_http_response_extract_body(wire).empty());
}

TEST(BalansunHttpResponseLogic, ChunkedRejectsTruncatedPayload) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "10\r\n"
      "short"
      "\r\n";
  EXPECT_TRUE(balansun_http_response_extract_body(wire).empty());
}

TEST(BalansunHttpResponseLogic, ChunkedRejectsMissingChunkCrlf) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "5\r\n"
      "hello";
  EXPECT_TRUE(balansun_http_response_extract_body(wire).empty());
}

TEST(BalansunHttpResponseLogic, ChunkedRejectsMissingSizeLineCrlf) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "5";
  EXPECT_TRUE(balansun_http_response_extract_body(wire).empty());
}

TEST(BalansunHttpResponseLogic, ChunkedConcatenatesMultipleChunks) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "2\r\n"
      "ab"
      "\r\n"
      "2\r\n"
      "cd"
      "\r\n"
      "0\r\n"
      "\r\n";
  EXPECT_EQ(balansun_http_response_extract_body(wire), "abcd");
}

TEST(BalansunHttpResponseLogic, HeaderScanStopsOnLastLineWithoutCrlf) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding: chunked\r\n"
      "X-Ignore: y\r\n"
      "\r\n"
      "0\r\n"
      "\r\n";
  EXPECT_TRUE(balansun_http_response_extract_body(wire).empty());
}

TEST(BalansunHttpResponseLogic, ChunkedHeaderRequiresColonAfterName) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "2\r\n"
      "ok"
      "\r\n"
      "0\r\n"
      "\r\n";
  EXPECT_EQ(balansun_http_response_extract_body(wire), "ok");
}

TEST(BalansunHttpResponseLogic, ChunkedRejectsLfOnlyAfterChunkData) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "2\r\n"
      "ab"
      "\n"
      "0\r\n"
      "\r\n";
  EXPECT_TRUE(balansun_http_response_extract_body(wire).empty());
}

TEST(BalansunHttpResponseLogic, ChunkedRejectsCrOnlyAfterChunkData) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "2\r\n"
      "ab"
      "\r"
      "0\r\n"
      "\r\n";
  EXPECT_TRUE(balansun_http_response_extract_body(wire).empty());
}

TEST(BalansunHttpResponseLogic, ChunkedEmptyBodyReturnsEmpty) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n";
  EXPECT_TRUE(balansun_http_response_extract_body(wire).empty());
}

TEST(BalansunHttpResponseLogic, ExtractTrimsTrailingCrOnly) {
  const std::string wire =
      "HTTP/1.1 200 OK\r\n"
      "\r\n"
      "x\r";
  EXPECT_EQ(balansun_http_response_extract_body(wire), "x");
}
