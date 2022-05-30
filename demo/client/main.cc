#include <stdlib.h>
#include <time.h>

#include <iostream>

#include "client.h"

int main(int , char *argv[]) {
  srand(time(NULL));

  // char log_dir[118];
  // snprintf(log_dir, 118, "%s/log/xmit/client", getenv("HOME"));
  // char mkdir[128];
  // snprintf(mkdir, 128, "mkdir -p %s", log_dir);
  // if (system(mkdir) == -1) {
  //  return false;
  // }
  // FLAGS_log_dir = log_dir;
  // google::InitGoogleLogging(argv[0]);

  xmit::demo::Client client(argv[0]);
  client.Start();

  // google::FlushLogFiles(google::INFO);

  return 0;
}
