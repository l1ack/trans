#pragma once

#include <list>
#include <stdint.h>
#include <unordered_map>

#include "que/event/loop.h"
#include "que/zmq/socket.h"

namespace xmit {
namespace demo {

class Server
  : private que::zmq::Socket::Visitor {
 public:
  Server(const std::string &path);
  ~Server();

 public:   // Start Server
  void Start();

 private:  // Stop Server
  void Stop();

 private:  // Handle Data
  void Echo();

  void Close(uint16_t cnid);

  void ProcessData(char *data, int size, uint16_t type);

  void ProcessCtrl(char *data, int size, uint16_t cnid);

  void Clear();

 private:  // Signal Channel
  void InitOfferLoop();
  void StopOfferLoop();

 private:  // Echo Server
  void InitEchoLoop();
  void StartEchoLoop();
  void StopEchoLoop();

 private:  // Establish Connection
  void RecvOffer(const std::string &conn_token);

 private:
  virtual void OnSocketMessage(que::zmq::Socket *socket, uint32_t conn,
                               char *buff, size_t size) override;

 private:
  struct Connection {
    ~Connection();

    void *server;
    std::string token;
    void *ctrl_stream;
    void *data_stream;
  };

 private:
  void *core_;
  que::event::Context *ctx_;

  void *server_;
  std::unordered_map<uint16_t, Connection *> running_conns_;
  std::list<std::pair<int32_t, Connection *> > closing_conns_;

  bool stopped_ = false;

  void *recv_msg_;

  que::event::Loop offer_loop_;

  que::event::Loop echo_loop_;

  que::event::Timer timer_;

  que::zmq::Socket *signal_channel_;
};

}  // namespace demo
}  // namespace xmit
