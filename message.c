#include "message.h"

void initMessageStruct(struct message * this, int type, int length, char * payload){
  this->type = type;
  this->length = length;
  this->payload = payload;
}

void sendMessageStruct(struct message * this, struct PortableSocket * reciever){
  char header[10];
  memset(header, 0 , 10);
  sprintf(header,"%d %d",this->type,this->length);
  cpSend(reciever, header, 10);
  cpSend(reciever, this->payload, this->length);
}

int recvMessageStruct(struct message * this, struct PortableSocket * sender){
  char header[10];
  memset(header, 0, 10);
  cpRecv(sender, header, 10);
  int type;
  int length;
  sscanf(header,"%d %d",&type,&length);
  if(length > 0) {
    int messagelength = cpRecv(sender, this->payload, length);
  }
  initMessageStruct(this,type,length,this->payload);
  printf("Recieved message of type %d length %d payload = %s\n", type,length,this->payload);
}
