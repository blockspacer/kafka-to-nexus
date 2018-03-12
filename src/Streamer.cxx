#include "Streamer.h"

#include "helper.h"
#include "logger.h"

#include <algorithm>
#include <memory>

namespace FileWriter {
std::chrono::milliseconds systemTime() {
  using namespace std::chrono;
  system_clock::time_point now = system_clock::now();
  return duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
}
} // namespace FileWriter

FileWriter::Streamer::Streamer(const std::string &Broker,
                               const std::string &TopicName,
                               const FileWriter::StreamerOptions &Opts)
    : RunStatus{ SEC::not_initialized }, Options(Opts) {

  if (TopicName.empty() || Broker.empty()) {
    LOG(Sev::Error, "Broker and topic required");
    RunStatus = SEC::not_initialized;
    return;
  }

  Options.Settings.ConfigurationStrings["group.id"] = TopicName;
  Options.Settings.ConfigurationStrings["auto.create.topics.enable"] = "false";
  Options.Settings.Address = Broker;

  ConnectThread = std::thread([&] {
    this->connect(std::ref(TopicName));
    return;
  });

  std::unique_lock<std::mutex> lk(ConnectionLock);
  ConnectionInit.wait(lk, [&] { return this->Initialising.load(); });
}

FileWriter::Streamer::~Streamer() { closeStream(); }

/// Create a RdKafka::Consumer a vector containing all the TopicPartition for
/// the given topic. If a start time is specified retrieve the correct initial
/// log. Assign the TopicPartition vector to the Consumer
/// \param TopicName the topic that the Streamer will consume
/// \param Options a StreamerOptions object
/// that contains configuration parameters for the Streamer and the KafkaConfig
void FileWriter::Streamer::connect(const std::string &TopicName) {
  std::lock_guard<std::mutex> lock(ConnectionReady);

  std::lock_guard<std::mutex> lk(ConnectionLock);
  Initialising = true;
  ConnectionInit.notify_all();

  LOG(Sev::Debug, "Connecting to {}", TopicName);
  try {
    ConsumerW.reset(new KafkaW::Consumer(Options.Settings));

    if (Options.StartTimestamp.count()) {
      ConsumerW->addTopic(TopicName,
                          Options.StartTimestamp - Options.BeforeStartTime);
    } else {
      ConsumerW->addTopic(TopicName);
    }
    // if the topic cannot be found in the metadata sets an error flag
    if (!ConsumerW->topicPresent(TopicName)) {
      RunStatus = SEC::topic_partition_error;
      return;
    }
    LOG(Sev::Debug, "Connected to topic {}", TopicName);
    RunStatus = SEC::writing;
  }
  catch (std::exception &e) {
    LOG(Sev::Error, "{}", e.what());
    RunStatus = SEC::configuration_error;
  }
}

FileWriter::Streamer::SEC FileWriter::Streamer::closeStream() {
  std::lock_guard<std::mutex> lock(ConnectionReady);
  if (ConnectThread.joinable()) {
    ConnectThread.join();
  }
  RunStatus = SEC::has_finished;
  return RunStatus;
}

template <>
FileWriter::ProcessMessageResult
FileWriter::Streamer::write(FileWriter::DemuxTopic &MessageProcessor) {

  if (RunStatus == SEC::not_initialized) {
    return ProcessMessageResult::OK();
  }
  std::lock_guard<std::mutex> lock(
      ConnectionReady); // make sure that connect is completed

  if (int(RunStatus) < 0) {
    return ProcessMessageResult::ERR();
  }
  if (RunStatus == SEC::has_finished) {
    return ProcessMessageResult::STOP();
  }

  KafkaW::PollStatus Poll = ConsumerW->poll();

  if (Poll.isEmpty() || Poll.isEOP()) {
    if ((Options.StopTimestamp.count() > 0) &&
        (systemTime() > (Options.StopTimestamp + Options.AfterStopTime))) {
      LOG(Sev::Info, "Close topic {} after time expired",
          MessageProcessor.topic());
      Sources.clear();
      return ProcessMessageResult::STOP();
    }
    return ProcessMessageResult::OK();
  }
  if (Poll.isErr()) {
    return ProcessMessageResult::ERR();
  }

  Msg Message(Msg::fromKafkaW(std::move(Poll.isMsg())));
  if (Message.type == MsgType::Invalid) {
    return ProcessMessageResult::ERR();
  }

  DemuxTopic::DT MessageTime =
      MessageProcessor.time_difference_from_message(Message);

  // size_t MessageLength = msg->len();
  // if the source is not in the source_list return OK (ignore)
  // if StartTimestamp is set and timestamp < start_time skip message and
  // return
  // OK, if StopTimestamp is set, timestamp > stop_time and the source is
  // still
  // present remove source and return STOP else process the message
  if (std::find(Sources.begin(), Sources.end(), MessageTime.sourcename) ==
      Sources.end()) {
    return ProcessMessageResult::OK();
  }
  if (MessageTime.dt < std::chrono::duration_cast<std::chrono::nanoseconds>(
                           Options.StartTimestamp).count()) {
    return ProcessMessageResult::OK();
  }
  if (Options.StopTimestamp.count() > 0 &&
      MessageTime.dt > std::chrono::duration_cast<std::chrono::nanoseconds>(
                           Options.StopTimestamp).count()) {
    if (removeSource(MessageTime.sourcename)) {
      return ProcessMessageResult::STOP();
    }
    return ProcessMessageResult::ERR();
  }

  // Collect information about the data received
  MessageInfo.message(Message.size());

  // Write the message. Log any error and return the result of processing
  ProcessMessageResult result =
      MessageProcessor.process_message(std::move(Message));
  LOG(Sev::Debug, "Processed: {}::{}\tpulse_time: {}", MessageProcessor.topic(),
      MessageTime.sourcename, result.ts());
  if (!result.is_OK()) {
    MessageInfo.error();
  }
  return result;
}

void FileWriter::Streamer::setSources(
    std::unordered_map<std::string, Source> &SourceList) {
  for (auto &Src : SourceList) {
    LOG(Sev::Info, "Add {} to source list", Src.first);
    Sources.push_back(Src.first);
  }
}

bool FileWriter::Streamer::removeSource(const std::string &SourceName) {
  auto Iter(std::find<std::vector<std::string>::iterator>(
      Sources.begin(), Sources.end(), SourceName));
  if (Iter == Sources.end()) {
    LOG(Sev::Warning, "Can't remove source {}, not in the source list",
        SourceName);
    return false;
  }
  Sources.erase(Iter);
  LOG(Sev::Info, "Remove source {}", SourceName);
  return true;
}

/// Method that parse the json configuration and parse the options to be used in
/// RdKafka::Config
void
FileWriter::StreamerOptions::setRdKafkaOptions(const rapidjson::Value *Opt) {

  if (!Opt->IsObject()) {
    LOG(Sev::Warning, "Unable to parse steamer options");
    return;
  }

  for (auto &m : Opt->GetObject()) {
    if (m.value.IsString()) {
      Settings.ConfigurationStrings[m.name.GetString()] = m.value.GetString();
      continue;
    }
    if (m.value.IsInt()) {
      Settings.ConfigurationIntegers[m.name.GetString()] = m.value.GetInt();
      continue;
    }
  }
}

/// Method that parse the json configuration and sets the parameters used in the
/// Streamer
void
FileWriter::StreamerOptions::setStreamerOptions(const rapidjson::Value *Opt) {

  if (!Opt->IsObject()) {
    LOG(Sev::Warning, "Unable to parse steamer options");
    return;
  }

  for (auto &m : Opt->GetObject()) {
    if (m.name.IsString()) {
      if (strncmp(m.name.GetString(), "ms-before-start", 15) == 0) {
        if (m.value.IsInt()) {
          LOG(Sev::Info, "Set {}: {}", m.name.GetString(), m.value.GetInt());
          BeforeStartTime = std::chrono::milliseconds(m.value.GetInt());
          continue;
        }
        LOG(Sev::Warning, "{} : wrong format", m.name.GetString());
      }
      if (strncmp(m.name.GetString(), "ms-after-stop", 13) == 0) {
        if (m.value.IsInt()) {
          LOG(Sev::Info, "Set {}: {}", m.name.GetString(), m.value.GetInt());
          AfterStopTime = std::chrono::milliseconds(m.value.GetInt());
          continue;
        }
        LOG(Sev::Warning, "{} : wrong format", m.name.GetString());
      }
      if (strncmp(m.name.GetString(), "consumer-timeout-ms", 19) == 0) {
        if (m.value.IsInt()) {
          LOG(Sev::Info, "Set {}: {}", m.name.GetString(), m.value.GetInt());
          ConsumerTimeout = std::chrono::milliseconds(m.value.GetInt());
          continue;
        }
        LOG(Sev::Warning, "{} : wrong format", m.name.GetString());
      }
      if (strncmp(m.name.GetString(), "metadata-retry", 14) == 0) {
        if (m.value.IsInt()) {
          LOG(Sev::Info, "Set {}: {}", m.name.GetString(), m.value.GetInt());
          NumMetadataRetry = m.value.GetInt();
          continue;
        }
        LOG(Sev::Warning, "{} : wrong format", m.name.GetString());
      }
      LOG(Sev::Warning, "Unknown option {}, ignore", m.name.GetString());
    }
  }
}
