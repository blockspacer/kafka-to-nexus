// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "DemuxTopic.h"

namespace FileWriter {

DemuxTopic::DemuxTopic(std::string TopicName) : Topic(std::move(TopicName)) {}

DemuxTopic::~DemuxTopic() { Logger->trace("DemuxTopic destructor"); }

DemuxTopic::DemuxTopic(DemuxTopic &&x) noexcept { swap(*this, x); }

void swap(DemuxTopic &x, DemuxTopic &y) {
  std::swap(x.Topic, y.Topic);
  std::swap(x.TopicSources, y.TopicSources);
}

std::string const &DemuxTopic::topic() const { return Topic; }

void DemuxTopic::process_message(FlatbufferMessage const &Message) {
  Logger->trace("Message received from: {}", Message.getSourceName());

  auto ProcessingResult = ProcessMessageResult::OK;

  try {
    auto &CurrentSource = TopicSources.at(Message.getSourceHash());
    ProcessingResult = CurrentSource.process_message(Message);
    ++messages_processed;
  } catch (std::out_of_range &e) {
    Logger->trace(R"(Source with name "{}" and ID "{}" is not in list.)",
                  Message.getSourceName(), Message.getFlatbufferID());
    ++error_no_source_instance;
    ProcessingResult = ProcessMessageResult::ERR;
  }

  if (ProcessingResult == ProcessMessageResult::ERR) {
    throw MessageProcessingException("Could not process message");
  }
}

std::unordered_map<FlatbufferMessage::SrcHash, Source> &DemuxTopic::sources() {
  return TopicSources;
}

} // namespace FileWriter
