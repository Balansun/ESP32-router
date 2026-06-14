#pragma once

#include <ctime>

/** False until wall clock is synced to a sane epoch (year >= 2020). */
bool balansun_time_is_valid(time_t now);

/** Decihours from hour/minute (e.g. 06:00 → 600). */
int balansun_wall_decihours_from_hm(int hour, int minute);

/** Local wall decihours from epoch seconds and GMT offset (for schedule periods). */
int balansun_wall_decihours_from_time(time_t now, int gmt_offset_sec);
