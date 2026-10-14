/* C library functions the payload SDK's libc does not provide, for a PS5 title. */
#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* The C locale is the only one in a title: one byte per character. */
int ___mb_cur_max(void) { return 1; }

/* Advice only: ignoring it is always allowed. */
int posix_madvise(void *addr, size_t len, int advice) {
    (void)addr; (void)len; (void)advice;
    return 0;
}

/* Days since 1970-01-01 for a proleptic Gregorian date (Howard Hinnant's days_from_civil). */
static int64_t days_from_civil(int64_t y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int64_t)doe - 719468;
}

/* mktime for UTC: struct tm fields as given, out-of-range months folded into years. */
time_t timegm(struct tm *tm) {
    int64_t year = (int64_t)tm->tm_year + 1900;
    int64_t month = tm->tm_mon;
    year += month / 12;
    month %= 12;
    if (month < 0) {
        month += 12;
        year -= 1;
    }
    int64_t days = days_from_civil(year, (unsigned)month + 1, 1) + (tm->tm_mday - 1);
    return (time_t)(days * 86400 + (int64_t)tm->tm_hour * 3600 + (int64_t)tm->tm_min * 60 + tm->tm_sec);
}
