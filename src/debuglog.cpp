#include "debuglog.h"
#include <Arduino.h>
#include <stdarg.h>

static portMUX_TYPE logMux = portMUX_INITIALIZER_UNLOCKED;
static char lines[DLOG_LINES][DLOG_COLS + 1];
static int head = 0, count = 0;  // head = baris paling lama
static volatile unsigned seq = 0;

void dlog(const char *fmt, ...) {
  char buf[DLOG_COLS + 1];
  int n = snprintf(buf, sizeof(buf), "%lu ", (unsigned long)(millis() / 1000));
  if (n < 0 || n >= (int)sizeof(buf)) n = 0;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
  va_end(ap);
  for (char *p = buf; *p; p++)
    if (*p < 32 || *p > 126) *p = '?';  // font OLED hanya ASCII
  portENTER_CRITICAL(&logMux);
  if (count == DLOG_LINES) { head = (head + 1) % DLOG_LINES; count--; }
  memcpy(lines[(head + count) % DLOG_LINES], buf, sizeof(buf));
  count++;
  seq++;
  portEXIT_CRITICAL(&logMux);
}

int dlog_count() { return count; }
unsigned dlog_seq() { return seq; }

void dlog_line(int i, char *out, size_t len) {
  portENTER_CRITICAL(&logMux);
  if (i >= 0 && i < count) strlcpy(out, lines[(head + i) % DLOG_LINES], len);
  else if (len) out[0] = '\0';
  portEXIT_CRITICAL(&logMux);
}

void dlog_clear() {
  portENTER_CRITICAL(&logMux);
  head = count = 0;
  portEXIT_CRITICAL(&logMux);
}
