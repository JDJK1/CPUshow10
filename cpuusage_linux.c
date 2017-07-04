#include <stdio.h>
#include <fcntl.h>
#include <math.h>

char cpuusage()
{
  static unsigned long prev_user_system_nice_time;
  static unsigned long prev_idle_time;
  unsigned long user_system_nice_time, idle_time, a[3];

  FILE* fp = fopen("/proc/stat", "r");
  fscanf(fp, "%*s %lu %lu %lu %lu", &a[0], &a[1], &a[2], &idle_time);
  user_system_nice_time = a[0] + a[1] + a[2];
  fclose(fp);

  char loadavg = lround(100.0 * (user_system_nice_time - prev_user_system_nice_time) /
    ((user_system_nice_time + idle_time) - (prev_user_system_nice_time + prev_idle_time)));

  prev_user_system_nice_time = user_system_nice_time;
  prev_idle_time = idle_time;

  return loadavg;
}



