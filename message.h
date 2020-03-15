#ifndef MESSAGE_HEADER
#define MESSAGE_HEADER
#define HEARTBEAT 0
#define MESSAGE 1
#define NEW_CONNECTION 2
#define RECONNECT 3

#include <string.h>
#include <stdio.h>
#include "portablesocket.h"

typedef struct message
{
  int type;
  int length;
  char * payload;
} message;

void createMessage(struct message * this, int type, int length, char * payload);

void sendMessageStruct(struct message * this, struct PortableSocket * reciever);

void recvMessageStruct(struct message * this, struct PortableSocket * sender);

#endif
