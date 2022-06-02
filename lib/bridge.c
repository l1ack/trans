#include "bridge.h"

#include <stdio.h>
#include <string.h>

#include "interface.h"

extern void goOnLog(const char *log);
void XmitOnLog(const char *log) {
  goOnLog(log);
}

void *XmitCore(int workers) {
  void *core = xmit_core_new(0);
  xmit_on_log(core, XmitOnLog, "XMIT LOG");
  return core;
}

int XmitStart(void *core) {
  return xmit_core_spawn(core);
}

int XmitStop(void *core) {
  return xmit_core_stop(core);
}

int XmitJoin(void *core) {
  return xmit_core_join(core);
}

void *XmitStream(void *core, unsigned short type,
                 int mode, int priority, int lifetime_ms) {
  return xmit_stream_new(core, type,
                         XMIT_MODE_SEMI_RELIABLE_ORDERED,
                         XMIT_PRIORITY_NORMAL, lifetime_ms);
}

void *XmitInitSendmsg(void *stream,
                      char *data, int size) {
  char *copy;
  void *msg = XmitInitSendmsgAllocBuffer(stream, &copy, size);
  memcpy(copy, data, size);
  return msg;
}

void *XmitInitSendmsgAllocBuffer(void *stream,
                                 char **data, int size) {
  void *msg = xmit_stream_msg(stream);
  xmit_msg_init_size(msg, size);
  *data = xmit_msg_data(msg);
  return msg;
}

void *XmitInitRecvmsg(void *core, void *msg) {
  if (!msg) {
    return xmit_msg_new(core);
  }
  xmit_msg_init(msg);
  return msg;
}

void *XmitServer(void *core, unsigned int public_address,
                 unsigned short port_start, unsigned short port_end,
                 char *visitor) {
  void *server = xmit_server_new(core, public_address,
                                 port_start, port_end);
  struct XmitServerCallbacks *callbacks = xmit_server_cbs(server);
  callbacks->on_message = XmitServerOnMessage;
  callbacks->on_connected = XmitServerOnConnected;
  callbacks->on_interrupted = XmitServerOnInterrupted;
  callbacks->on_failtoconnect = XmitServerOnFailtoconnect;
  callbacks->visitor = visitor;
  return server;
}

int XmitServerAccept(void *server, int transport,
                     char *conn_token, int token_size,
                     char *offer_data, int offer_size,
                     char *answer_data, int answer_size,
                     unsigned short *cnid) {
  return xmit_server_accept(server, transport,
                            conn_token, token_size,
                            offer_data, offer_size,
                            answer_data, answer_size,
                            0, cnid);
}

int XmitServerClose(void *server,
                     char *conn_token, int token_size) {
  return xmit_server_close(server, conn_token, token_size);
}

int XmitServerBind(void *server, void *stream,
                    char *conn_token, int token_size) {
  return xmit_server_bind(server, stream, conn_token, token_size);
}

int XmitServerSend(void *server, void *msg) {
  int sent = xmit_server_send(server, msg);
  xmit_msg_del(msg);
  return sent;
}

int XmitServerRecv(void *server, void *msg,
                   char **data, unsigned short *type,
                   unsigned short *cnid, char **conn_token, int *token_size) {
  xmit_server_recv(server, msg);
  *data = xmit_msg_data(msg);
  *type = xmit_msg_type(msg);
  *cnid = xmit_msg_conn(msg, NULL, 0);
  return xmit_msg_size(msg);
}

extern void goServerOnMessage(void *visitor, void *data, int size, unsigned short type, unsigned short cnid);
void XmitServerOnMessage(void *visitor, void *msg) {
  goServerOnMessage(visitor,
                    xmit_msg_data(msg), xmit_msg_size(msg),
                    xmit_msg_type(msg), xmit_msg_conn(msg, NULL, 0));
}

extern void goServerOnConnected(void *visitor, unsigned short cnid);
void XmitServerOnConnected(void *visitor, unsigned short cnid) {
  goServerOnConnected(visitor, cnid);
}

extern void goServerOnInterrupted(void *visitor, unsigned short cnid,
                                  int interval_ms);
void XmitServerOnInterrupted(void *visitor, unsigned short cnid,
                             int interval_ms) {
  goServerOnInterrupted(visitor, cnid, interval_ms);
}

extern void goServerOnFailtoconnect(void *visitor, unsigned short cnid);
void XmitServerOnFailtoconnect(void *visitor, unsigned short cnid) {
  goServerOnFailtoconnect(visitor, cnid);
}

void *XmitClient(void *core, int transport, char *visitor) {
  void *client = xmit_client_new(core, transport);
  struct XmitClientCallbacks *callbacks = xmit_client_cbs(client);
  callbacks->on_message = XmitClientOnMessage;
  callbacks->on_connected = XmitClientOnConnected;
  callbacks->on_interrupted = XmitClientOnInterrupted;
  callbacks->on_failtoconnect = XmitClientOnFailtoconnect;
  callbacks->visitor = visitor;
  return client;
}

int XmitClientAccept(void *client, int *transport,
                     char *answer_data, int answer_size,
                     uint32_t *address, uint16_t *port) {
  return xmit_client_accept(client, transport,
                            answer_data, answer_size,
                            address, port);
}

int XmitClientBind(void *client, void *stream) {
  return xmit_client_bind(client, stream);
}

int XmitClientSend(void *client, void *msg) {
  int sent = xmit_client_send(client, msg);
  xmit_msg_del(msg);
  return sent;
}

int XmitClientRecv(void *client, void *msg,
                   char **data, unsigned short *type) {
  xmit_client_recv(client, msg);
  *data = xmit_msg_data(msg);
  *type = xmit_msg_type(msg);
  return xmit_msg_size(msg);
}

extern void goClientOnMessage(void *visitor, void *data, int size, unsigned short type, unsigned short cnid);
void XmitClientOnMessage(void *visitor, void *msg) {
  goClientOnMessage(visitor,
                    xmit_msg_data(msg), xmit_msg_size(msg),
                    xmit_msg_type(msg), xmit_msg_conn(msg, NULL, 0));
}

extern void goClientOnConnected(void *visitor);
void XmitClientOnConnected(void *visitor) {
  goClientOnConnected(visitor);
}

extern void goClientOnInterrupted(void *visitor,
                                  int interval_ms);
void XmitClientOnInterrupted(void *visitor,
                             int interval_ms) {
  goClientOnInterrupted(visitor, interval_ms);
}

extern void goClientOnFailtoconnect(void *visitor);
void XmitClientOnFailtoconnect(void *visitor) {
  goClientOnFailtoconnect(visitor);
}

void *XmitWdcClient(void *client, char *sender) {
  void *wdc = xmit_wdc_for_client(client);
  struct XmitWdcSender *wdc_sender =
    (struct XmitWdcSender *)xmit_wdc_get_sender(wdc);
  wdc_sender->on_send = XmitWdcOnSend;
  wdc_sender->sender = sender;
  return wdc;
}

void *XmitWdcServer(void *server, char *sender) {
  void *wdc = xmit_wdc_for_server(server);
  struct XmitWdcSender *wdc_sender = xmit_wdc_get_sender(wdc);
  wdc_sender->on_send = XmitWdcOnSend;
  wdc_sender->sender = sender;
  return wdc;
}

int XmitWdcOnRecv(void *recver,
                  char *buff, int size, unsigned short dcid) {
  return xmit_wdc_onrecv(recver, buff, size, dcid);
}

extern void goWdcOnSend(void *sender, char *buff, int size, unsigned short dcid);
void XmitWdcOnSend(void *sender,
                   char *buff, int size, unsigned short dcid) {
  goWdcOnSend(sender, buff, size, dcid);
}


void *XmitSignalNew(void *core) {
  return xmit_signal_new(core);
}

void *XmitSignalServer(void *signal,
                       unsigned short port) {
  return xmit_signal_server(signal, port);
}

void *XmitSignalClient(void *signal,
                       unsigned int address, unsigned short port) {
  return xmit_signal_client(signal, address, port);
}

int XmitSignalSendOffer(void *client,
                        char *buff, int size) {
  return xmit_signal_send_offer(client, buff, size);
}

int XmitSignalRecvOffer(void *server,
                        char *buff, int size) {
  return xmit_signal_recv_offer(server, buff, size);
}

int XmitSignalSendAnswer(void *server,
                         char *buff, int size) {
  return xmit_signal_send_answer(server, buff, size);
}

int XmitSignalRecvAnswer(void *client,
                         char *buff, int size) {
  return xmit_signal_recv_answer(client, buff, size);
}

int XmitSignalJoin(void *signal) {
  return xmit_signal_join(signal);
}

int XmitSignalDel(void *signal, void *client, void *server) {
  return xmit_signal_del(signal, client, server);
}
