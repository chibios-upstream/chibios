/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "cmdutil.h"

static char get_type_char(mode_t mode) {

  if (S_ISDIR(mode)) {
    return 'd';
  }
  if (S_ISCHR(mode)) {
    return 'c';
  }
  if (S_ISBLK(mode)) {
    return 'b';
  }
  if (S_ISFIFO(mode)) {
    return 'p';
  }
#if defined(S_ISLNK)
  if (S_ISLNK(mode)) {
    return 'l';
  }
#endif
#if defined(S_ISSOCK)
  if (S_ISSOCK(mode)) {
    return 's';
  }
#endif

  return '-';
}

/* Days from 1970-01-01 to a civil date, proleptic Gregorian calendar.*/
static void civil_from_days(long long days, long long *yearp,
                            unsigned *monthp, unsigned *dayp) {
  long long era;
  unsigned doe, yoe, doy, mp;

  days += 719468;
  era = ((days >= 0) ? days : (days - 146096)) / 146097;
  doe = (unsigned)(days - (era * 146097));
  yoe = (doe - (doe / 1460U) + (doe / 36524U) - (doe / 146096U)) / 365U;
  doy = doe - ((365U * yoe) + (yoe / 4U) - (yoe / 100U));
  mp = ((5U * doy) + 2U) / 153U;
  *dayp = doy - (((153U * mp) + 2U) / 5U) + 1U;
  *monthp = (mp < 10U) ? (mp + 3U) : (mp - 9U);
  *yearp = (long long)yoe + (era * 400) + ((*monthp <= 2U) ? 1 : 0);
}

int cmdWriteAll(int fd, const void *buf, size_t count) {
  const unsigned char *p;

  p = (const unsigned char *)buf;
  while (count > 0U) {
    ssize_t n;

    n = write(fd, p, count);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (n == 0) {
      errno = EIO;
      return -1;
    }
    p += (size_t)n;
    count -= (size_t)n;
  }

  return 0;
}

void cmdReportError(const char *command, const char *operand) {

  fprintf(stderr, "%s: %s: %s%s",
          command, operand, strerror(errno), CMD_NEWLINE_STR);
}

int cmdParseUnsigned(const char *text, unsigned long maximum,
                     unsigned long *valuep) {
  unsigned long value;
  char *endp;

  if ((*text == '\0') || (*text == '-')) {
    return -1;
  }

  errno = 0;
  value = strtoul(text, &endp, 10);
  if ((errno == ERANGE) || (*endp != '\0') || (value > maximum)) {
    return -1;
  }

  *valuep = value;
  return 0;
}

/* Type and permissions as in "ls -l", CMD_MODE_SIZE bytes.*/
void cmdFormatMode(mode_t mode, char *buf) {

  buf[0]  = get_type_char(mode);
  buf[1]  = ((mode & S_IRUSR) != 0) ? 'r' : '-';
  buf[2]  = ((mode & S_IWUSR) != 0) ? 'w' : '-';
  buf[3]  = ((mode & S_IXUSR) != 0) ? 'x' : '-';
  buf[4]  = ((mode & S_IRGRP) != 0) ? 'r' : '-';
  buf[5]  = ((mode & S_IWGRP) != 0) ? 'w' : '-';
  buf[6]  = ((mode & S_IXGRP) != 0) ? 'x' : '-';
  buf[7]  = ((mode & S_IROTH) != 0) ? 'r' : '-';
  buf[8]  = ((mode & S_IWOTH) != 0) ? 'w' : '-';
  buf[9]  = ((mode & S_IXOTH) != 0) ? 'x' : '-';
  buf[10] = '\0';
}

/* UTC time as "YYYY-MM-DD HH:MM" or "YYYY-MM-DD HH:MM:SS",
   CMD_TIME_SIZE bytes.*/
void cmdFormatTime(time_t t, bool seconds, char *buf) {
  long long secs, days, year;
  unsigned month, day;

  secs = (long long)t;
  days = secs / 86400;
  secs = secs % 86400;
  if (secs < 0) {
    secs += 86400;
    days--;
  }
  civil_from_days(days, &year, &month, &day);
  if (seconds) {
    (void)snprintf(buf, CMD_TIME_SIZE, "%04ld-%02u-%02u %02u:%02u:%02u",
                   (long)year, month, day, (unsigned)(secs / 3600),
                   (unsigned)((secs / 60) % 60), (unsigned)(secs % 60));
  }
  else {
    (void)snprintf(buf, CMD_TIME_SIZE, "%04ld-%02u-%02u %02u:%02u",
                   (long)year, month, day, (unsigned)(secs / 3600),
                   (unsigned)((secs / 60) % 60));
  }
}
