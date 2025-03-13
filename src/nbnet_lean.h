#pragma once

#include "stdint.h"

extern "C" {

typedef uint32_t NBN_ConnectionHandle;

typedef struct NBN_MessageInfo {
  /** User defined message's type */
  uint8_t type;

  /** Channel the message was received on */
  uint8_t channel_id;

  /** Message's data */
  void *data;

  /**
   * The message's sender.
   *
   * On the client side, it will always be 0 (all received messages come from
   * the game server).
   */
  NBN_ConnectionHandle sender;
} NBN_MessageInfo;

#define NBN_BYTE_ARRAY_MAX_SIZE 4096

#define NBN_MAX_MESSAGE_TYPES 255
#define NBN_BYTE_ARRAY_MESSAGE_TYPE (NBN_MAX_MESSAGE_TYPES - 4)

typedef struct NBN_ByteArrayMessage {
  uint8_t bytes[NBN_BYTE_ARRAY_MAX_SIZE];
  unsigned int length;
} NBN_ByteArrayMessage;

void NBN_GameClient_Stop(void);
int NBN_GameClient_Start(const char *protocol_name, const char *host,
                         uint16_t port);

#define NBN_NO_EVENT 0   /* No event left in the events queue */
#define NBN_SKIP_EVENT 1 /* Indicates that the event should be skipped */
#define NBN_EVENT_QUEUE_CAPACITY 1024
int NBN_GameClient_Poll(void);

NBN_MessageInfo NBN_GameClient_GetMessageInfo(void);

int NBN_GameClient_SendPackets(void);

void NBN_ByteArrayMessage_Destroy(NBN_ByteArrayMessage *msg);
int NBN_GameClient_SendReliableByteArray(uint8_t *bytes, unsigned int length);
enum {
  /* Client is connected to server */
  NBN_CONNECTED = 2,

  /* Client is disconnected from the server */
  NBN_DISCONNECTED,

  /* Client has received a message from the server */
  NBN_MESSAGE_RECEIVED
};
}
