// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "ThreadedExecutor.h"
#include "MessageWriter.h"
#include "FlatbufferMessage.h"
#include "Message.h"
#include <chrono>
#include "KafkaW/Consumer.h"
#include "PartitionFilter.h"
#include "SourceFilter.h"
#include "Stream/MessageWriter.h"

namespace Stream {

// Pollution of namespace, fix.
using SrcToDst = std::vector<std::pair<FileWriter::FlatbufferMessage::SrcHash, Message::DstId>>;
using std::chrono_literals::operator ""ms;
using std::chrono_literals::operator ""s;
using time_point = std::chrono::system_clock::time_point;

class Partition {
public:
  Partition() = default;

  Partition(std::unique_ptr<KafkaW::Consumer> Consumer, SrcToDst Map, MessageWriter *Writer,
            Metrics::Registrar RegisterMetric,
            time_point Start, time_point Stop = time_point::max());

  void setStopTime(time_point Stop);

  bool hasFinished();

protected:
  Metrics::Metric KafkaTimeouts{"timeouts",
                                "Timeouts when polling for messages."};
  Metrics::Metric KafkaErrors{"kafka_errors",
                              "Errors received when polling for messages.",
                              Metrics::Severity::ERROR};
  Metrics::Metric MessagesReceived{"received",
                                   "Number of messages received from broker."};
  Metrics::Metric MessagesProcessed{"processed",
                                    "Number of messages queued up for writing."};
  Metrics::Metric BadOffsets{"bad_offsets",
                             "Number of messages received with bad offsets.", Metrics::Severity::ERROR};

  Metrics::Metric FlatbufferErrors{"flatbuffer_errors",
                              "Errors when creating flatbuffer message from Kafka message.",
                              Metrics::Severity::ERROR};

  Metrics::Metric BadTimestamps{"bad_timestamps",
                                "Number of messages received with bad timestamps.", Metrics::Severity::ERROR};

  void pollForMessage();

  void processMessage(FileWriter::Msg const &Message);

  std::unique_ptr<KafkaW::Consumer> ConsumerPtr;
  std::atomic_bool HasFinished{false};
  std::int64_t CurrentOffset{0};
  ThreadedExecutor Executor; //Must be last
  PartitionFilter StopTester;
  std::map<FileWriter::FlatbufferMessage::SrcHash, SourceFilter> MsgFilters;
};

} // namespace Stream
