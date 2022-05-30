#pragma once

#include <list>
#include <map>
#include <stdint.h>
#include <unordered_map>

#include "que/event/loop.h"
#include "que/zmq/socket.h"

#include "protocol/api/interface.h"

namespace xmit {
namespace demo {

class Client
  : private que::zmq::Socket::Visitor {
 public:
  Client(const std::string &path);
  ~Client();

 public:   // Start Client
  void Start();

 private:  // Stop Client
  void Stop();

 private:  // Close Connection
  void Close();

 private:
  void SetCallbacks();

 private:  // Handle Data
  void Recv(void *msg);

  void Send();

  void ProcessCtrl(char *data, int size);

  void ProcessData(char *data, int size);

  void PrepareSize(int *size);

  void PrepareData(char *data, int size);

 private:  // Manage Connection
  void RecvAnswer(char *buff, size_t size);

  void SendCloseSignal();

 private:  // Sender
  void InitSendLoop();
  void StartSendLoop();
  void StopSendLoop();

 private:
  struct Message {
    uint64_t seq;
    char *data;
    int size;
    int64_t timestamp;
  };

 private:
  void Check();

  void Lost(const Message &msg);

  void SendOffer();

 private:
  virtual void OnSocketMessage(que::zmq::Socket *socket, uint32_t conn,
                               char *buff, size_t size) override;

 private:
  static const int kRoundNumber;
  static const int kBatchSize;
  static const int kRoundIntervalMs;
  static const int kMinFrameSize;
  static const int kMaxFrameSize;

 private:
  void *core_;
  que::event::Context *ctx_;

  std::string conn_token_;

  void *client_;
  void *ctrl_stream_;
  void *data_stream_;

  int64_t close_at_ = 0;

  uint64_t send_seq_ = 0;
  uint64_t recv_seq_ = 0;
  std::unordered_map<uint64_t, Message> send_msgs_;
  que::event::Loop send_loop_;
  que::event::Timer send_timer_;

  int round_ = 0;

  que::zmq::Socket *signal_channel_;

  std::map<int64_t, std::list<uint64_t> > timeouts_;
};

}  // namespace demo
}  // namespace xmit
