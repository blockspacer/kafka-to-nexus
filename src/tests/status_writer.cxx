#include <gtest/gtest.h>
#include <random>

#include <Status.hpp>
#include <StatusWriter.h>

#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/writer.h"

using MessageInfo = FileWriter::Status::MessageInfo;
using StreamMasterInfo = FileWriter::Status::StreamMasterInfo;
using StreamWriter = FileWriter::Status::JSONStreamWriter;

template <class T> class TD;

constexpr int n_messages{100};

TEST(StatusWriter, create_document) {
  StreamMasterInfo info;
  StreamWriter writer;
  auto value = writer.write(info);
  EXPECT_TRUE(value.size() > 0);
}

TEST(StatusWriter, create_report_streamers) {
  std::default_random_engine generator;
  std::normal_distribution<double> normal(0.0, 1.024e6);

  StreamMasterInfo info;
  const std::vector<std::string> topics{"first", "second", "third"};

  info.time_to_next_message(milliseconds(2000));
  for (auto &t : topics) {
    MessageInfo mi;
    for (int i = 0; i < n_messages; ++i) {
      if (i % 10) {
        auto message_size = std::fabs(normal(generator));
        mi.message(message_size);
      } else {
        mi.error();
      }
    }
    info.add(t, mi);
  }

  StreamWriter writer;
  auto s = std::move(writer.write(info));
  //  std::cout << writer.write(info) << "\n";

  // ASSERT_EQ(s,expect);
}
