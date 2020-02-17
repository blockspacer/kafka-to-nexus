// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include <chrono>
#include "KafkaW/BrokerSettings.h"
#include "Partition.h"
#include "ThreadedExecutor.h"
#include "logger.h"
#include "Metrics/Registrar.h"

namespace Stream {

class Topic {
public:
  Topic(KafkaW::BrokerSettings Settings, std::string Topic,
        time_point StartTime, Metrics::Registrar &RegisterMetric);

  void setStopTime(std::chrono::system_clock::time_point StopTime);

  ~Topic() = default;

protected:
  time_point BeginConsumeTime;
  std::chrono::system_clock::duration CurrentMetadataTimeOut;
  Metrics::Registrar Registrar;

  void
  getPartitionsForTopic(KafkaW::BrokerSettings Settings, std::string Topic);

  void
  getOffsetsForPartitions(KafkaW::BrokerSettings Settings, std::string Topic,
                          std::vector<int> Partitions);

  void createStreams(KafkaW::BrokerSettings Settings, std::string Topic,
                     std::vector<std::pair<int, int64_t>> PartitionOffsets);

  std::vector<std::unique_ptr<Partition>> ConsumerThreads;
  ThreadedExecutor Executor; // Must be last
};
} // namespace Stream