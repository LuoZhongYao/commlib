/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#define LOG_TAG "btsnoop"

#include <btsnoop.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <commlib/Log.h>
#include <pthread.h>

#define TRANSPORT_CHANNEL_CMD                (1)
#define TRANSPORT_CHANNEL_ACL                (2)
#define TRANSPORT_CHANNEL_SCO                (3)
#define TRANSPORT_CHANNEL_EVT                (4)

enum {
  kCommandPacket = 1,
  kAclPacket = 2,
  kScoPacket = 3,
  kEventPacket = 4
};

// Epoch in microseconds since 01/01/0000.
static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;
struct btsnoop
{
    int fd;
    pthread_mutex_t mutex;
};

static uint64_t btsnoop_timestamp(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);

  // Timestamp is in microseconds.
  uint64_t timestamp = tv.tv_sec * 1000 * 1000LL;
  timestamp += tv.tv_usec;
  timestamp += BTSNOOP_EPOCH_DELTA;
  return timestamp;
}

void btsnoop_capturev(struct btsnoop *btsnoop, const struct iovec iov[], size_t iovn, bool is_received)
{
    int length_he = 0;
    int length;
    int flags;
    int drops = 0;

    if(btsnoop && btsnoop->fd >= 0 && iovn >= 1) {
        uint8_t *head = iov[0].iov_base;
        switch(*head) {
        case kCommandPacket:
            length_he = head[3] + 4;
            flags = 2;
        break;

        case kAclPacket:
            flags = is_received;
            length_he = (head[4] << 8) + head[3] + 5;
        break;

        case kScoPacket:
            flags = is_received;
            length_he = head[3] + 4;
        break;

        case kEventPacket:
            length_he = head[2] + 3;
            flags = 3;
        break;
        }

        uint64_t timestamp = btsnoop_timestamp();
        uint32_t time_hi = timestamp >> 32;
        uint32_t time_lo = timestamp & 0xFFFFFFFF;

        length = htonl(length_he);
        flags = htonl(flags);
        drops = htonl(drops);
        time_hi = htonl(time_hi);
        time_lo = htonl(time_lo);

        pthread_mutex_lock(&btsnoop->mutex);
        write(btsnoop->fd, &length, 4);
        write(btsnoop->fd, &length, 4);
        write(btsnoop->fd, &flags, 4);
        write(btsnoop->fd, &drops, 4);
        write(btsnoop->fd, &time_hi, 4);
        write(btsnoop->fd, &time_lo, 4);
        writev(btsnoop->fd, iov, iovn);
        pthread_mutex_unlock(&btsnoop->mutex);
    }
}

static void btsnoop_write_packet(struct btsnoop *btsnoop, int type, const uint8_t *packet, bool is_received) {
  int length_he = 0;
  int length;
  int flags;
  int drops = 0;
  switch (type) {
    case kCommandPacket:
      length_he = packet[2] + 4;
      flags = 2;
      break;
    case kAclPacket:
      length_he = (packet[3] << 8) + packet[2] + 5;
      flags = is_received;
      break;
    case kScoPacket:
      length_he = packet[2] + 4;
      flags = is_received;
      break;
    case kEventPacket:
      length_he = packet[1] + 3;
      flags = 3;
      break;
  }

  uint64_t timestamp = btsnoop_timestamp();
  uint32_t time_hi = timestamp >> 32;
  uint32_t time_lo = timestamp & 0xFFFFFFFF;

  length = htonl(length_he);
  flags = htonl(flags);
  drops = htonl(drops);
  time_hi = htonl(time_hi);
  time_lo = htonl(time_lo);

  // This function is called from different contexts.
  //utils_lock();
  pthread_mutex_lock(&btsnoop->mutex);

  write(btsnoop->fd, &length, 4);
  write(btsnoop->fd, &length, 4);
  write(btsnoop->fd, &flags, 4);
  write(btsnoop->fd, &drops, 4);
  write(btsnoop->fd, &time_hi, 4);
  write(btsnoop->fd, &time_lo, 4);
  write(btsnoop->fd, &type, 1);
  write(btsnoop->fd, packet, length_he - 1);

  pthread_mutex_unlock(&btsnoop->mutex);
  //utils_unlock();
}

struct btsnoop *btsnoop_open(const char *p_path)
{
    struct btsnoop *btsnoop;
    int  fd;
    assert(p_path != NULL);
    assert(*p_path != '\0');

    fd = open(p_path,
              O_WRONLY | O_CREAT | O_TRUNC,
              S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

    if (fd == -1) {
        BLOGE("%s unable to open '%s': %s", __func__, p_path, strerror(errno));
        return NULL;
    }

    write(fd, "btsnoop\0\0\0\0\1\0\0\x3\xea", 16);
    btsnoop = malloc(sizeof(struct btsnoop));
    btsnoop->fd = fd;
    pthread_mutex_init(&btsnoop->mutex, NULL);
    return btsnoop;
}

void btsnoop_close(struct btsnoop *btsnoop)
{
    if(btsnoop) {
        pthread_mutex_destroy(&btsnoop->mutex);
        if(btsnoop->fd != -1)
            close(btsnoop->fd);
        free(btsnoop);
    }
}

void btsnoop_capture(struct btsnoop *btsnoop, int channel, const void *p, bool is_rcvd)
{

    if (btsnoop && btsnoop->fd >= 0) {
        btsnoop_write_packet(btsnoop, channel, p, is_rcvd);
    }
}
