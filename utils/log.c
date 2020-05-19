#include "./log.h"

const char* path_tree = "/home/dominator/shitcodes/c-c++/acronis/differentiator/log/tree.log";
const char* path_diff = "/home/dominator/shitcodes/c-c++/acronis/differentiator/log/diff.log";

int fd_tree = -1;
int fd_diff = -1;

char* GetTimestamp() {
  time_t    ltime;
  struct tm result;
  char      date[11];
  char      hrs[9];

  ltime = time(NULL);
  localtime_r(&ltime, &result);
  strftime(date, 11, "%F", &result);
  strftime(hrs, 9, "%X", &result);
  char* log_name = calloc(22, sizeof(char));
  sprintf(log_name, "[%s|%s]", date, hrs);

  return log_name;
}
