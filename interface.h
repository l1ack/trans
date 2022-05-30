#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XMIT_CORE_BLOCK_SIZE    0
#define XMIT_CORE_HEADER_SPACE  1

#define XMIT_MODE_NONE_RELIABLE            0
#define XMIT_MODE_FULL_RELIABLE            1
#define XMIT_MODE_SEMI_RELIABLE_UNORDERED  2
#define XMIT_MODE_SEMI_RELIABLE_ORDERED    3

#define XMIT_PRIORITY_LOW       0
#define XMIT_PRIORITY_NORMAL    1
#define XMIT_PRIORITY_HIGH      2
#define XMIT_PRIORITY_CRITICAL  3

//  ---
//  API
//  ---

//  create a xmit core, which manages work threads, memory allocation, event loops, etc.
//                      both server and client(s) use the same core instance
//  argument: (threads)                  - number of work threads, if thread = 0, a default value (1) is used
//  return: newly created core
void *xmit_core_new(int workers);

//  get core option
//  argument: (option)                   - option name
//  return: option value, or -1 on error
int xmit_core_get(void *core,
                  int option);

//  set core option
//  argument: (option)                   - option name
//            (value)                    - option value
//  return: -1 on error
int xmit_core_set(void *core,
                  int option, int value);

//  run xmit on foreground
int xmit_core_start(void *core);

//  run xmit on background
int xmit_core_spawn(void *core);

//  join xmit threads (if xmit core were spawned, not started)
int xmit_core_join(void *core);

//  create an empty msg for receving
//  return: the msg
void *xmit_msg_new(void *core);

//  get type of a msg
//  return: the type
uint16_t xmit_msg_type(void *msg);

//  get the data buff of a msg
//  return: data buff
char *xmit_msg_data(void *msg);

//  get the data size of a msg
//  return: data size
int xmit_msg_size(void *msg);

//  init an empty msg
//  return: -1 on error
int xmit_msg_init(void *msg);

//  init the msg with a buffer (data) of size (size), the buffer will not be freed by xmit
//  argument: (data)                     - msg data buff
//            (size)                     - msg data size
//  return: data size, or -1 on error
int xmit_msg_init_data(void *msg,
                       char *data, int size);

//  init the msg with a buffer of size (size), which can be filled with data to send
//  argument: (size)                     - msg data size
//  return: data size, or -1 on error
int xmit_msg_init_size(void *msg,
                       int size);

//  move data of a message to another message
//  argument: (dst)                      - destination msg
//            (src)                      - source msg
//  return: data size, or -1 on error
int xmit_msg_move_data(void *dst, void *src);

//  get connection bind with msg
//  argument: (conn_token)               - will point to the token
//  return: token length
uint16_t xmit_msg_conn(void *msg,
                       char **conn_token, int *token_size);

//  get message ID
//  return: message ID
uint64_t xmit_msg_id(void *msg);

//  check if a message is a key frame
//  return: 1 if is key frame, 0 if not, -1 on error
int xmit_msg_is_key(void *msg);

//  set message as a key frame
int xmit_msg_set_key(void *msg);

//  add a reference msg, at most 3 ref can be added to a message
//  argument: (ref_id)                   - reference message ID
//  return: -1 on error, like if number of reference  exceeded 3
int xmit_msg_add_ref(void *msg, uint64_t ref_id);

//  get references of a msg
//  argument: (ref_id)                   - reference message ID
//            (ref_id)                   - index of reference, can not be larger then 3
//  return: number of reference, or -1 on error
int xmit_msg_get_ref(void *msg, uint64_t *ref_id, int index);

//  destroy a msg
//  return: -1 on error
int xmit_msg_del(void *msg);

//  create a stream to send
//  argument: (mode), (priority), (lifetime_ms) - stream properties
//  return: the stream
void *xmit_stream_new(void *core, uint16_t type,
                      int mode, int priority, int lifetime_ms);

//  create a msg from a stream, the stream have to be bound to a server or client before creating any msg
//  return: the msg
void *xmit_stream_msg(void *stream);

//  get connection bind with stream
//  argument: (conn_token)               - will point to the token
//  return: token length
int xmit_stream_conn(void *stream,
                     char **conn_token);

//  destroy a stream
//  return: -1 on error
int xmit_stream_del(void *stream);

//  create a server, may (and should) share the same core with client (if there is a client)
//  argument: (public_address),          - host order, if you guys want to use string, let me(Qi Ji) know
//            ([start_port, end_port))   - binding port range, will try from start until success
//  return: newly created server
void *xmit_server_new(void *core,
                      uint32_t public_address,
                      uint16_t start_port, uint16_t end_port);
void *xmit_server_dev(void *core,
                      uint32_t public_address,
                      uint16_t start_port, uint16_t end_port,
                      int loss, int delay, int jitter);

//  get callbacks for server
void *xmit_server_cbs(void *server);

//  get core of a server
void *xmit_server_core(void *server);

//  create an answer
//  argument: (conn_token), (token_size) - the connection is identified by a c-style string: (conn_token), with length of (token_size)
//                                         the string is NOT null terminated
//            (timeout_ms_)              - connection timeout(ms), failtoconnect/interrupted callback interval, if 0 is provided, default value is set to be 3000(ms)
//            (answer_buff), (answer_size) - where the created answer is stored, the answer_buff should be larger then answer_size
//  return: actual answer size
int xmit_server_accept(void *server, int protocol,
                       char *conn_token, int token_size,
                       char *offer_buff, int offer_size,
                       char *answer_buff, int answer_size,
                       int timeout_ms, uint16_t *cnid);

//  bind a stream to a server connection, once a stream is bound, it's permanent
//                                        you can NOT close it or alter its properties
//                                        just close the connection if you want
//  argument: (conn_token), (token_size) - the connection identifier
//  return: -1 on error
int xmit_server_bind(void *server,
                     void *stream,
                     char *conn_token, int token_size);

//  close a connection
//  argument: (conn_token), (token_size) - the connection identifier
int xmit_server_close(void *server,
                      char *conn_token, int token_size);

//  send a message
//  argument: (message)                  - pointer to the message struct
int xmit_server_send(void *server,
                     void *msg);

//  received a message
//  argument: (message)                  - pointer to the message struct
//  return: number of bytes actually received, or -1 if error happened
int xmit_server_recv(void *server,
                     void *msg);

//  release the Server
//  argument: (server) - created by xmit_server_new
int xmit_server_del(void *server);

//  create a client, may (and should) share the same core with client (if there is a client)
//  argument: ([start_port, end_port)) - binding port range, will try from start until success
//  return: newly created client
void *xmit_client_new(void *core,
                      int transport);
void *xmit_client_dev(void *core,
                      int transport, int loss, int delay, int jitter);

//  get callbacks for client
void *xmit_client_cbs(void *client);

//  get core of a client
void *xmit_client_core(void *client);

//  accept an answer
//  argument: (answer_buff), (answer_size) - the answer
//  return: -1 on error, or the carried answer size
int xmit_client_accept(void *client, int *transport,
                       char *answer_buff, int answer_size,
                       unsigned int *address, unsigned short *port);

//  bind a stream to a client connection, once a stream is bound, it's permanent
//                                        you can NOT close it or alter its properties
//                                        just close the connection if you want
//  return: -1 on error
int xmit_client_bind(void *client,
                     void *stream);

//  close the connection
int xmit_client_close(void *client);

//  send a message
//  argument: (message)                  - pointer to the message struct
//  return: message ID
int xmit_client_send(void *client,
                     void *msg);

//  received a message
//  argument: (message)                  - pointer to the message struct
//  return: number of bytes actually received, or -1 if error happened
int xmit_client_recv(void *client,
                     void *msg);

//  release the client
//  argument: (client) - created by xmit_client_new
int xmit_client_del(void *client);

//  stop the core
//  argument: (core) - created by xmit_core_new
int xmit_core_stop(void *core);

//  close and release the core
//  argument: (core) - created by xmit_core_new
int xmit_core_del(void *core);

//  ---------
//  callbacks
//  ---------

// utils
typedef void (*xmit_log_callback) (const char *log);  // null terminated

int xmit_on_log(void *core, xmit_log_callback cb, const char *prefix);

// server
typedef void (*xmit_server_on_message) (void *visitor, void *msg);

typedef void (*xmit_server_on_connected) (void *visitor,
                                          uint16_t conn_id);

typedef void (*xmit_server_on_interrupted) (void *visitor,
                                            uint16_t conn_id,
                                            int interval_ms);

typedef void (*xmit_server_on_failtoconnect) (void *visitor,
                                              uint16_t conn_id);

typedef struct XmitServerCallbacks {
  xmit_server_on_message on_message;
  xmit_server_on_connected on_connected;
  xmit_server_on_interrupted on_interrupted;
  xmit_server_on_failtoconnect on_failtoconnect;

  void *visitor;
} xmit_server_callbacks;

// client
typedef void (*xmit_client_on_message) (void *visitor, void *msg);

typedef void (*xmit_client_on_connected) (void *visitor);

typedef void (*xmit_client_on_interrupted) (void *visitor,
                                            int interval_ms);

typedef void (*xmit_client_on_failtoconnect) (void *visitor);

typedef struct XmitClientCallbacks {
  xmit_client_on_message on_message;
  xmit_client_on_connected on_connected;
  xmit_client_on_interrupted on_interrupted;
  xmit_client_on_failtoconnect on_failtoconnect;

  void *visitor;
} xmit_client_callbacks;

//  ---------
//  transport
//  ---------

typedef void (*xmit_wdc_onsend) (void *sender,
                                 char *buff, int size, uint16_t dcid);

typedef struct XmitWdcSender {
  xmit_wdc_onsend on_send;

  void *sender;
} xmit_wdc_sender;

void *xmit_wdc_for_client(void *client);

void *xmit_wdc_for_server(void *server);

void *xmit_wdc_get_sender(void *wdc);

int xmit_wdc_onrecv(void *wdc,
                    char *buff, int size, uint16_t dcid);

//  -------
//  private
//  -------

//  signaling utilities
void *xmit_signal_new(void *core);

void *xmit_signal_server(void *signal,
                         uint16_t port);

void *xmit_signal_client(void *signal,
                         uint32_t address, uint16_t port);

int xmit_signal_send_offer(void *client,
                           char *buff, int size);

int xmit_signal_recv_offer(void *server,
                           char *buff, int size);

int xmit_signal_send_answer(void *server,
                            char *buff, int size);

int xmit_signal_recv_answer(void *client,
                            char *buff, int size);

int xmit_signal_join(void *signal);

int xmit_signal_del(void *signal, void *client, void *server);

#ifdef __cplusplus
}
#endif
