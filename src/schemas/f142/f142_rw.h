#include "../../FlatbufferReader.h"
#include "../../HDFWriterModule.h"
#include "../../h5.h"
#include "WriterTypedBase.h"
#include <array>
#include <flatbuffers/flatbuffers.h>
#include <memory>
#include <vector>

template <typename T> using uptr = std::unique_ptr<T>;

namespace FileWriter {
namespace Schemas {
namespace f142 {

#include "schemas/f142_logdata_generated.h"

using FBUF = LogData;

template <typename DT, typename FV> class WriterArray : public WriterTypedBase {
public:
  WriterArray(hdf5::node::Group hdf_group, std::string const &source_name,
              hsize_t ncols, Value fb_value_type_id, CollectiveQueue *cq);
  WriterArray(hdf5::node::Group, std::string const &source_name, hsize_t ncols,
              Value fb_value_type_id, CollectiveQueue *cq,
              HDFIDStore *hdf_store);
  h5::append_ret write_impl(FBUF const *fbuf) override;
  uptr<h5::h5d_chunked_2d<DT>> ds;
  Value _fb_value_type_id = Value::NONE;
};

template <typename DT, typename FV>
class writer_typed_scalar : public WriterTypedBase {
public:
  writer_typed_scalar(hdf5::node::Group hdf_group,
                      std::string const &source_name, Value fb_value_type_id,
                      CollectiveQueue *cq);
  writer_typed_scalar(hdf5::node::Group hdf_group,
                      std::string const &source_name, Value fb_value_type_id,
                      CollectiveQueue *cq, HDFIDStore *hdf_store);
  h5::append_ret write_impl(FBUF const *fbuf) override;
  uptr<h5::h5d_chunked_1d<DT>> ds;
  Value _fb_value_type_id = Value::NONE;
};

class FlatbufferReader : public FileWriter::FlatbufferReader {
  bool verify(Msg const &msg) const override;
  std::string source_name(Msg const &msg) const override;
  uint64_t timestamp(Msg const &msg) const override;
};

class HDFWriterModule : public FileWriter::HDFWriterModule {
public:
  static FileWriter::HDFWriterModule::ptr create();
  InitResult init_hdf(hdf5::node::Group &HDFGroup,
                      std::string const &HDFAttributes) override;
  void parse_config(std::string const &ConfigurationStream,
                    std::string const &ConfigurationModule) override;
  HDFWriterModule::InitResult reopen(hdf5::node::Group &HDFGroup) override;
  void enable_cq(CollectiveQueue *cq, HDFIDStore *hdf_store,
                 int mpi_rank) override;
  WriteResult write(Msg const &msg) override;
  int32_t flush() override;
  int32_t close() override;

  uptr<WriterTypedBase> impl;
  uptr<h5::h5d_chunked_1d<uint64_t>> ds_timestamp;
  uptr<h5::h5d_chunked_1d<uint64_t>> ds_cue_timestamp_zero;
  uptr<h5::h5d_chunked_1d<uint64_t>> ds_cue_index;
  uptr<h5::h5d_chunked_1d<uint64_t>> ds_seq_data;
  uptr<h5::h5d_chunked_1d<uint64_t>> ds_seq_fwd;
  uptr<h5::h5d_chunked_1d<uint64_t>> ds_ts_data;
  bool do_flush_always = false;
  bool do_writer_forwarder_internal = false;
  uint64_t total_written_bytes = 0;
  uint64_t index_at_bytes = 0;
  // set by default to a large value:
  uint64_t index_every_bytes = !0;
  uint64_t ts_max = 0;
  size_t array_size = 0;
  std::string source_name;
  std::string type;
  CollectiveQueue *cq = nullptr;
};
}
}
}
