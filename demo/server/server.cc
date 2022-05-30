#include "server.h"

#include <cassert>
#include <fstream>
#include <iostream>

#include "que/core/core.h"
#include "que/event/loop_pool.h"
#include "que/json/json.h"
#include "que/network/address.h"
#include "que/system/clock.h"
#include "que/util/debug.h"
#include "que/zmq/endpoint.h"

#include "protocol/common/consts.h"
#include "protocol/api/interface.h"

#include "demo/common/consts.h"

namespace xmit {
namespace demo {

Server::Connection::~Connection() {
  xmit_stream_del(ctrl_stream);
  xmit_stream_del(data_stream);
  xmit_server_close(server, token.data(), token.size());
}

Server::Server(const std::string &path)
  : core_(xmit_core_new(1)),
    ctx_(((que::Core *)core_)->GetContext()),
    recv_msg_(xmit_msg_new(core_)),
    offer_loop_(ctx_),
    echo_loop_(ctx_),
    signal_channel_(ctx_->NewRepSocket(this)) {
  uint32_t server_address;
  if (path.empty()) {
    server_address = que::network::kLoopbackAddress;
  } else {
    auto config_path = path;
    config_path = config_path.replace(path.size() - 6, 6, "config.json");
    std::ifstream config_fs(config_path, std::ifstream::binary);
    auto config = json::parse(config_fs);

    if (!que::network::StringToIp(config["server"], &server_address)) {
      std::cout << "Invalid Config, fail to read server address: "
                << config["server"] << std::endl;
      exit(-1);
    }
  }

  server_ = xmit_server_new(core_, server_address, 9130, 60000);
  // server_ = xmit_server_dev(core_, 0, 9130, 9131, 8, 16, 4),

  char endpoint[128];
  que::zmq::TcpEndpoint(0u, 50913, 0u, 0, endpoint);
  signal_channel_->Bind(endpoint);
  offer_loop_.Push(signal_channel_);
}

Server::~Server() {
  xmit_msg_del(recv_msg_);
  for (auto &conn : running_conns_) {
    delete conn.second;
  }
  for (auto &conn : closing_conns_) {
    delete conn.second;
  }
  xmit_server_del(server_);
  delete signal_channel_;

  xmit_core_del(core_);
  std::cout << "Stop Server" << std::endl;
}

void Server::Start() {
  std::cout << "Start Server" << std::endl;
  xmit_core_spawn(core_);

  InitOfferLoop();

  InitEchoLoop();
  StartEchoLoop();

  offer_loop_.Join();
  echo_loop_.Join();
  xmit_core_join(core_);
}

void Server::Stop() {
  stopped_ = true;

  StopEchoLoop();

  StopOfferLoop();

  xmit_core_stop(core_);
}

void Server::Echo() {
  xmit_msg_init(recv_msg_);
  xmit_server_recv(server_, recv_msg_);
  uint16_t send_type = xmit_msg_type(recv_msg_);
  char *send_data = xmit_msg_data(recv_msg_);
  int send_size = xmit_msg_size(recv_msg_);
  auto cnid = xmit_msg_conn(recv_msg_, nullptr, nullptr);
  auto conn_iter = running_conns_.find(cnid);
  if (conn_iter == running_conns_.end()) {
    return;
  }
  auto conn = conn_iter->second;

  void *stream;
  switch (send_type) {
  case kCtrlStream:
    stream = conn->ctrl_stream;
    ProcessCtrl(send_data, send_size, cnid);
    break;
  case kDataStream:
    stream = conn->data_stream;
    ProcessData(send_data, send_size, send_type);
    break;
  default:
    std::cout << "ERROR, unknown stream type: " << send_type << std::endl;
    exit(-1);
  }
  auto send_msg = xmit_stream_msg(stream);
  xmit_msg_move_data(send_msg, recv_msg_);
  xmit_server_send(server_, send_msg);
  xmit_msg_del(send_msg);
}

void Server::Close(uint16_t cnid) {
  auto conn_iter = running_conns_.find(cnid);
  if (conn_iter == running_conns_.end()) {
    std::cout << "ERROR: " << "fail to find connection: "
              << cnid << " to close" << std::endl;
    return;
  }
  auto now = que::system::TickInSeconds();
  closing_conns_.emplace_back(now + 3, conn_iter->second);
  running_conns_.erase(conn_iter);
}

void Server::Clear() {
  auto now = que::system::TickInSeconds();
  for (auto &conn : closing_conns_) {
    if (conn.first > now) {
      break;
    }
    std::cout << "Close Connection: " << conn.first << std::endl;
    delete conn.second;
    closing_conns_.pop_front();
  }
}

void Server::ProcessData(char *data, int size, uint16_t type) {
  auto echo_seq = *reinterpret_cast<uint64_t *>(&data[0]);
  std::cout << "Msg: (" << echo_seq << ", TYPE: " << type
            << ", SIZE: " << size << ") Echoed" << std::endl;
}

void Server::ProcessCtrl(char *data, int size, uint16_t cnid) {
  (void)size;
  auto signal = *reinterpret_cast<int *>(&data[0]);
  switch (signal) {
  case kCloseSignal:
    Close(cnid);
    break;
  default:
    std::cout << "ERROR, unknown control signal: " << signal << std::endl;
    exit(-1);
  }
}

void Server::InitOfferLoop() {
  offer_loop_.Spawn("offer");
}

void Server::StopOfferLoop() {
  offer_loop_.Stop();
}

void Server::InitEchoLoop() {
  echo_loop_.Spawn("echoer");
}

void Server::StartEchoLoop() {
  echo_loop_.AddTimer(&timer_, 500,
                      [](int, void *arg) {
                        reinterpret_cast<Server *>(arg)->Clear();
                      }, this);
  echo_loop_.Signal([](void *self) {
                      auto server = reinterpret_cast<Server *>(self);
                      while (true) {
                        server->Echo();
                      }
                    }, this);
}

void Server::StopEchoLoop() {
  echo_loop_.Stop();
  echo_loop_.CancelTimer(&timer_);
}

void Server::RecvOffer(const std::string &conn_token) {
  int answer_limit = 8192;
  char *answer_buff = new char[answer_limit];
  uint16_t conn_id;

  char *token_buff = const_cast<char *>(conn_token.data());
  size_t token_size = conn_token.size();
  auto answer_size =
    xmit_server_accept(server_, 0,
                       token_buff, token_size,
                       nullptr, 0,
                       answer_buff, answer_limit,
                       60 * 1000, &conn_id);

  auto ctrl_stream =
    xmit_stream_new(core_, kCtrlStream,
                    protocol::Mode::kSemiReliableOrdered,
                    protocol::Priority::kNormal, 0);
  xmit_server_bind(server_, ctrl_stream, token_buff, token_size);
  auto data_stream =
    xmit_stream_new(core_, kDataStream,
                    protocol::Mode::kSemiReliableOrdered,
                    protocol::Priority::kNormal, 0);
  xmit_server_bind(server_, data_stream, token_buff, token_size);
  auto conn = new Connection({server_, {token_buff, token_size},
                              ctrl_stream, data_stream});
  running_conns_.insert({conn_id, conn});

  int answer_sent = signal_channel_->Send(answer_buff, answer_size);
  que::util::PrintBuffer(answer_buff, answer_sent, std::cout << "Answer: ");
  delete[] answer_buff;
}

void Server::OnSocketMessage(que::zmq::Socket *, uint32_t,
                             char *buff, size_t size) {
  std::cout << "On Offer" << std::endl;
  RecvOffer({buff, size});
}

}  // namespace demo
}  // namespace xmit
