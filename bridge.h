void *XmitCore(int workers);

int XmitStart(void *core);

int XmitStop(void *core);

int XmitJoin(void *core);

void *XmitStream(void *core, unsigned short type,
                 int mode, int priority, int lifetime_ms);

void *XmitInitSendmsg(void *stream,
                      char *data, int size);

void *XmitInitSendmsgAllocBuffer(void *stream,
                                 char **data, int size);

void *XmitInitRecvmsg(void *core, void *msg);

void *XmitServer(void *core, unsigned int public_address,
                 unsigned short port_start, unsigned short port_end,
                 char *visitor);

int XmitServerAccept(void *server, int transport,
                     char *conn_token, int token_size,
                     char *offer_data, int offer_size,
                     char *answer_data, int answer_size,
                     unsigned short *cnid);

int XmitServerClose(void *server,
                    char *conn_token, int token_size);

int XmitServerBind(void *server, void *stream,
                   char *conn_token, int token_size);

int XmitServerSend(void *server, void *msg);

int XmitServerRecv(void *server, void *msg,
                   char **data, unsigned short *type,
                   unsigned short *cnid, char **conn_token, int *token_size);

void XmitServerOnMessage(void *visitor, void *msg);

void XmitServerOnConnected(void *visitor, unsigned short cnid);

void XmitServerOnInterrupted(void *visitor, unsigned short cnid,
                             int interval_ms);

void XmitServerOnFailtoconnect(void *visitor, unsigned short cnid);

void XmitServerOnLog(void *visitor, const char *log);

void *XmitClient(void *core, int transport, char *visitor);

int XmitClientAccept(void *client, int *transport,
                     char *answer_data, int answer_size,
                     unsigned int *address, unsigned short *port);

int XmitClientBind(void *client, void *stream);

int XmitClientSend(void *client, void *msg);

int XmitClientRecv(void *client, void *msg,
                   char **data, unsigned short *type);

void XmitClientOnMessage(void *visitor, void *msg);

void XmitClientOnConnected(void *visitor);

void XmitClientOnInterrupted(void *visitor,
                             int interval_ms);

void XmitClientOnFailtoconnect(void *visitor);

void XmitClientOnLog(void *visitor, const char *log);

void *XmitWdcClient(void *client, char *sender);

void *XmitWdcServer(void *server, char *sender);

int XmitWdcOnRecv(void *wdc,
                  char *buff, int size, unsigned short dcid);

void XmitWdcOnSend(void *sender,
                   char *buff, int size, unsigned short dcid);

void *XmitSignalNew(void *core);

void *XmitSignalServer(void *signal,
                       unsigned short port);

void *XmitSignalClient(void *signal,
                       unsigned int address, unsigned short port);

int XmitSignalSendOffer(void *client,
                        char *buff, int size);

int XmitSignalRecvOffer(void *server,
                        char *buff, int size);

int XmitSignalSendAnswer(void *server,
                         char *buff, int size);

int XmitSignalRecvAnswer(void *client,
                         char *buff, int size);

int XmitSignalJoin(void *signal);

int XmitSignalDel(void *signal, void *client, void *server);
