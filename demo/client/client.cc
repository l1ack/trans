#include "client.h"

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

const int Client::kRoundNumber = 333;
const int Client::kBatchSize = 1;
const int Client::kRoundIntervalMs = 333;
// const int Client::kMinFrameSize = 50000;
// const int Client::kMaxFrameSize = 100000;
const int Client::kMinFrameSize = 333;
const int Client::kMaxFrameSize = 333 * 1000;

Client::Client(const std::string &path)
  : core_(xmit_core_new(1)),
    ctx_(((que::Core *)core_)->GetContext()),
    client_(xmit_client_new(core_, 0)),
    // client_(xmit_client_dev(core_, 8, 16, 4)),
    ctrl_stream_(xmit_stream_new(core_, kCtrlStream,
                                 protocol::Mode::kSemiReliableOrdered,
                                 protocol::Priority::kNormal, 0)),
    data_stream_(xmit_stream_new(core_, kDataStream,
                                 protocol::Mode::kSemiReliableOrdered,
                                 protocol::Priority::kNormal, 0)),
    send_loop_(ctx_),
    signal_channel_(ctx_->NewReqSocket(this)) {
  uint32_t server_address;
  if (path.empty()) {
    server_address = que::network::kLoopbackAddress;
    conn_token_ = "demo:loop:back:conn";
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
    conn_token_ = config["token"];
    if (conn_token_.empty()) {
      std::cout << "Invalid Config, empty token" << std::endl;
      exit(-2);
    }
    if (conn_token_ == "null") {
      std::cout << "Invalid Config, token unspecified, "
                << "using of your MAC address is recommended!"
                << std::endl;
      exit(-3);
    }
  }
  int64_t now_ms = que::system::NowInMilliSeconds();
  conn_token_ += std::to_string(now_ms);

  SetCallbacks();

  // xmit_core_set(core_, XMIT_CORE_BLOCK_SIZE, 10272);
  char endpoint[128];
  que::zmq::TcpEndpoint(0u, 0, server_address, 50913, endpoint);
  signal_channel_->Connect(endpoint);
  send_loop_.Push(signal_channel_);
  SendOffer();
}

Client::~Client() {
  xmit_stream_del(ctrl_stream_);
  xmit_stream_del(data_stream_);
  xmit_client_del(client_);
  delete signal_channel_;

  xmit_core_del(core_);
  std::cout << "Stop Client" << std::endl;
}

void Client::SetCallbacks() {
  xmit_on_log(core_, [](const char *log) {
                       std::cout << log << std::endl;
                     }, "XMIT Demo Client LOG");

  auto callbacks =
    reinterpret_cast<XmitClientCallbacks *>(xmit_client_cbs(client_));
  callbacks->on_message = [](void *client, void *msg) {
                            reinterpret_cast<Client *>(client)->Recv(msg);
                          };
  callbacks->visitor = this;
}

void Client::Start() {
  std::cout << "Start Client" << std::endl;
  xmit_core_spawn(core_);

  InitSendLoop();

  xmit_core_join(core_);
}

void Client::Stop() {
  StopSendLoop();
  xmit_core_stop(core_);
}

void Client::Close() {
  close_at_ = que::system::TickInMilliSeconds() + 3000;
}

void Client::Recv(void *msg) {
  uint16_t recv_type = xmit_msg_type(msg);
  char *recv_data = xmit_msg_data(msg);
  int recv_size = xmit_msg_size(msg);
  switch (recv_type) {
  case kCtrlStream:
    ProcessCtrl(recv_data, recv_size);
    break;
  case kDataStream:
    ProcessData(recv_data, recv_size);
    break;
  default:
    std::cout << "ERROR, unknown stream type: " << recv_type << std::endl;
    exit(-1);
  }
}

void Client::Send() {
  if (close_at_) {
    return;
  }
  for (int i = 0; i < kBatchSize; i++) {
    int send_size = 0;
    PrepareSize(&send_size);
    auto send_msg = xmit_stream_msg(data_stream_);
    xmit_msg_init_size(send_msg, send_size);
    char *send_data = xmit_msg_data(send_msg);
    PrepareData(send_data, send_size);
    xmit_client_send(client_, send_msg);
    xmit_msg_del(send_msg);
  }
  if (++round_ == kRoundNumber) {
    Close();
  }
}

void Client::ProcessCtrl(char *data, int size) {
  (void)size;
  auto signal = *reinterpret_cast<int *>(&data[0]);
  switch (signal) {
  case kCloseSignal:
    Stop();
    break;
  default:
    std::cout << "ERROR, unknown control signal: " << signal << std::endl;
    exit(-1);
  }
}

void Client::ProcessData(char *data, int size) {
  auto recv_seq = *reinterpret_cast<uint64_t *>(&data[0]);
  if (recv_seq != recv_seq_) {
    std::cout << "Msg: [" << recv_seq_ << " ~ " << recv_seq
              <<  ") FAILED!" << std::endl;
  }
  recv_seq_ = recv_seq + 1;

  auto msg_iter = send_msgs_.find(recv_seq);
  if (msg_iter == send_msgs_.end()) {
    return;
  }

  auto &send_msg = msg_iter->second;
  int rtt = que::system::TickInMilliSeconds() - send_msg.timestamp;
  if (send_msg.size == size && !strncmp(send_msg.data, data, size)) {
    std::cout << "Msg: (" << send_msg.seq << ") Echoed, SIZE: " << size
              << ", RTT: " << rtt << std::endl;
  } else {
    std::cout << "Msg: (" << send_msg.seq << ") Corrupted, SIZE: " << size
              << "<received:sent>" << send_msg.size
              << ", RTT: " << rtt << std::endl;
  }
  send_msgs_.erase(msg_iter);
}

void Client::PrepareSize(int *size) {
  *size = (rand() % (kMaxFrameSize - kMinFrameSize)) + kMinFrameSize;
}

void Client::PrepareData(char *data, int size) {
  uint64_t sequence = send_seq_++;
  send_msgs_[sequence] = {sequence, data, size,
                          que::system::TickInMilliSeconds()};
  *reinterpret_cast<uint64_t *>(&data[0]) = sequence;
  std::cout << "Msg: (" << sequence << ") Sent: " << size << std::endl;
}

void Client::RecvAnswer(char *buff, size_t size) {
  xmit_client_accept(client_, nullptr, buff, size, nullptr, nullptr);
  xmit_client_bind(client_, ctrl_stream_);
  xmit_client_bind(client_, data_stream_);
  que::util::PrintBuffer(buff, size, std::cout << "Accept: ");

  StartSendLoop();
}

void Client::SendCloseSignal() {
  auto close_msg = xmit_stream_msg(ctrl_stream_);
  xmit_msg_init_size(close_msg, sizeof(int));
  char *close_signal = xmit_msg_data(close_msg);
  *reinterpret_cast<int *>(&close_signal[0]) = kCloseSignal;
  xmit_client_send(client_, close_msg);
  xmit_msg_del(close_msg);
}

void Client::InitSendLoop() {
  send_loop_.Spawn("sender");
}

void Client::StartSendLoop() {
  send_loop_.AddTimer(&send_timer_, kRoundIntervalMs,
                      [](int, void *arg) {
                        reinterpret_cast<Client *>(arg)->Send();
                        reinterpret_cast<Client *>(arg)->Check();
                      }, this);
}

void Client::StopSendLoop() {
  send_loop_.Stop();
  send_loop_.CancelTimer(&send_timer_);
}

void Client::Check() {
  auto now = que::system::TickInMilliSeconds();
  if (close_at_ && close_at_ < now) {
    SendCloseSignal();
    return;
  }

  while (timeouts_.size() && timeouts_.begin()->first < now) {
    for (uint16_t send_seq : timeouts_.begin()->second) {
      auto msg_iter = send_msgs_.find(send_seq);
      if (msg_iter != send_msgs_.end()) {
        Lost(msg_iter->second);
        send_msgs_.erase(msg_iter);
      }
    }
    timeouts_.erase(timeouts_.begin());
  }
}

void Client::Lost(const Message &msg) {
  std::cout << "Msg: (" << msg.seq << ") Lost, size: "
            << msg.size << std::endl;
  delete[] msg.data;
}

void Client::SendOffer() {
  std::cout << "sent offer: "
            << signal_channel_->Send(conn_token_.data(), conn_token_.size())
            << std::endl;
}

void Client::OnSocketMessage(que::zmq::Socket *, uint32_t,
                             char *buff, size_t size) {
  std::cout << "On Answer" << std::endl;
  RecvAnswer(buff, size);
}

}  // namespace demo
}  // namespace xmit
