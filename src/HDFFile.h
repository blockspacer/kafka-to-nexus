#pragma once
#include "CollectiveQueue.h"
#include "Jemalloc.h"
#include "Msg.h"
#include <H5Ipublic.h>
#include <memory>
#include <rapidjson/document.h>
#include <string>
#include <vector>

class T_HDFFile;

namespace FileWriter {

// POD
struct StreamHDFInfo {
  std::string hdf_parent_name;
  rapidjson::Value const *config_stream;
};

// Basically POD
class WriteResult {
public:
  int64_t ts;
};

class HDFFile final {
public:
  HDFFile();
  ~HDFFile();
  int init(std::string filename, rapidjson::Value const &nexus_structure,
           rapidjson::Value const &config_file,
           std::vector<StreamHDFInfo> &stream_hdf_info,
           std::vector<hid_t> &groups);
  int reopen(std::string filename, rapidjson::Value const &config_file);
  void flush();
  void create_collective_queue(Jemalloc::sptr jm);
  hid_t h5file = -1;
  CollectiveQueue::ptr cq;

private:
  friend class ::T_HDFFile;
  friend class CommandHandler;
};

} // namespace FileWriter
