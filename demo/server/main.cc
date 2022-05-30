#include <stdlib.h>
#include <time.h>

#include <iostream>

#include "que/network/address.h"

#include "server.h"

int main(int, char *argv[]) {
  srand(time(NULL));

  // char log_dir[118];
  // snprintf(log_dir, 118, "%s/log/xmit/server", getenv("HOME"));
  // char mkdir[128];
  // snprintf(mkdir, 128, "mkdir -p %s", log_dir);
  // if (system(mkdir) == -1) {
  //  return false;
  // }
  // FLAGS_log_dir = log_dir;
  // google::InitGoogleLogging(argv[0]);

  xmit::demo::Server server(argv[0]);
  server.Start();

  // google::FlushLogFiles(google::INFO);

  return 0;
}
