#include "message.h"

void createMessage(struct message *this, int type, int length, char *payload)
{
    this->type = type;
    this->length = length;
    this->payload = payload;
}

void sendMessageStruct(struct message *this, struct PortableSocket *reciever)
{
    char header[10];
    memset(header, 0, 10);
    sprintf(header, "%d %d", this->type, this->length);
    portableSend(reciever, header, 10);
    portableSend(reciever, this->payload, this->length);
}

void recvMessageStruct(struct message *this, struct PortableSocket *sender)
{
    char header[10];
    memset(header, 0, 10);
    portableRecv(sender, header, 10);
    int type;
    int length;
    sscanf(header, "%d %d", &type, &length);
    if (length > 0)
    {
        portableRecv(sender, this->payload, length);
    }
    createMessage(this, type, length, this->payload);
}
