#include "Msg.h"

#include <librdkafka/rdkafka.h>

namespace KafkaW {

Msg::~Msg() { rd_kafka_message_destroy((rd_kafka_message_t *)kmsg); }

uchar *Msg::data() { return (uchar *)((rd_kafka_message_t *)kmsg)->payload; }

uint32_t Msg::size() { return ((rd_kafka_message_t *)kmsg)->len; }

char const *Msg::topic_name() {
  return rd_kafka_topic_name(((rd_kafka_message_t *)kmsg)->rkt);
}

int32_t Msg::partition() { return ((rd_kafka_message_t *)kmsg)->partition; }

int32_t Msg::offset() { return ((rd_kafka_message_t *)kmsg)->offset; }
}
