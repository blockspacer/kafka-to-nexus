#pragma once

#include "FlatbufferReader.h"
#include "HDFFile.h"
#include "HDFWriterModule.h"
#include "Msg.h"
#include "ProcessMessageResult.h"
#include "TimeDifferenceFromMessage.h"
#include "json.h"
#include <string>

class Test___FileWriterTask___Create01;

namespace FileWriter {

class SourceFactory_by_FileWriterTask;

class Result {
public:
  static Result Ok();
  bool is_OK();
  bool is_ERR();

private:
  int _res = 0;
};

/// Represents a sourcename on a topic.
/// The sourcename can be empty.
/// This is meant for highest efficiency on topics which are exclusively used
/// for only one sourcename.
class Source {
public:
  Source(Source &&);
  ~Source();
  std::string const &topic() const;
  std::string const &source() const;
  /// If non-empty, specifies the broker where the data of this Source can be
  /// found.
  std::string const &broker() const;
  uint32_t processed_messages_count() const;
  ProcessMessageResult process_message(Msg const &msg);
  std::string to_str() const;
  rapidjson::Document
  to_json(rapidjson::MemoryPoolAllocator<> *a = nullptr) const;
  void config_file(rapidjson::Value const *config_file);
  void config_stream(rapidjson::Document &&config_stream);

private:
  Source(std::string topic, std::string source);

  std::string _topic;
  std::string _source;
  std::string _broker;
  std::string _hdf_path;
  std::unique_ptr<HDFWriterModule> _hdf_writer_module;

  uint64_t _processed_messages_count = 0;
  uint64_t _cnt_msg_written = 0;

  rapidjson::Value const *_config_file = nullptr;
  rapidjson::Document _config_stream;

  friend class CommandHandler;
  friend class FileWriterTask;
  friend class SourceFactory_by_FileWriterTask;
  friend class ::Test___FileWriterTask___Create01;
};

} // namespace FileWriter
