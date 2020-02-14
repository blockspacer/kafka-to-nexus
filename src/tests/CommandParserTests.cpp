// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "CommandParser.h"
#include <chrono>
#include <gtest/gtest.h>
#include <pl72_run_start_generated.h>

namespace {
flatbuffers::DetachedBuffer buildRunStartMessage(
    std::string const &InstrumentName, std::string const &RunName,
    std::string const &NexusStructure, std::string const &JobID,
    std::string const &ServiceID, std::string const &Broker,
    std::string const &Filename, uint64_t StartTime, uint64_t StopTime) {
  flatbuffers::FlatBufferBuilder Builder;

  const auto InstrumentNameOffset = Builder.CreateString(InstrumentName);
  const auto RunIDOffset = Builder.CreateString(RunName);
  const auto NexusStructureOffset = Builder.CreateString(NexusStructure);
  const auto JobIDOffset = Builder.CreateString(JobID);
  const auto ServiceIDOffset = Builder.CreateString(ServiceID);
  const auto BrokerOffset = Builder.CreateString(Broker);
  const auto FilenameOffset = Builder.CreateString(Filename);

  auto messageRunStart =
      CreateRunStart(Builder, StartTime, StopTime, RunIDOffset,
                     InstrumentNameOffset, NexusStructureOffset, JobIDOffset,
                     BrokerOffset, ServiceIDOffset, FilenameOffset);

  FinishRunStartBuffer(Builder, messageRunStart);
  return Builder.Release();
}

std::string const InstrumentNameInput = "TEST";
std::string const RunNameInput = "42";
std::string const NexusStructureInput = "{}";
std::string const JobIDInput = "qw3rty";
std::string const ServiceIDInput = "filewriter1";
std::string const BrokerInput = "somehost:1234";
std::string const FilenameInput = "a-dummy-name-01.h5";
uint64_t const StartTimeInput = 123456789000;
uint64_t const StopTimeInput = 123456790000;
} // namespace

class CommandParserHappyStartTests : public testing::Test {
public:
  FileWriter::StartCommandInfo StartInfo;

  // cppcheck-suppress unusedFunction
  void SetUp() override {
    auto MessageBuffer = buildRunStartMessage(
        InstrumentNameInput, RunNameInput, NexusStructureInput, JobIDInput,
        ServiceIDInput, BrokerInput, FilenameInput, StartTimeInput,
        StopTimeInput);

    StartInfo = FileWriter::CommandParser::extractStartInformation(
        MessageBuffer.data(), MessageBuffer.size());
  }
};

TEST_F(CommandParserHappyStartTests, IfJobIDPresentThenExtractedCorrectly) {
  ASSERT_EQ(JobIDInput, StartInfo.JobID);
}

TEST_F(CommandParserHappyStartTests, IfFilenamePresentThenExtractedCorrectly) {
  ASSERT_EQ(FilenameInput, StartInfo.Filename);
}

TEST_F(CommandParserHappyStartTests, IfBrokerPresentThenExtractedCorrectly) {
  ASSERT_EQ(JobIDInput, StartInfo.BrokerInfo.HostPort);
  ASSERT_EQ(1234u, StartInfo.BrokerInfo.Port);
}

TEST_F(CommandParserHappyStartTests,
       IfNexusStructurePresentThenExtractedCorrectly) {
  ASSERT_EQ(NexusStructureInput, StartInfo.NexusStructure);
}

TEST_F(CommandParserHappyStartTests, IfStartPresentThenExtractedCorrectly) {
  ASSERT_EQ(std::chrono::milliseconds{StartTimeInput}, StartInfo.StartTime);
}

TEST_F(CommandParserHappyStartTests, IfStopPresentThenExtractedCorrectly) {
  ASSERT_EQ(std::chrono::milliseconds{StopTimeInput}, StartInfo.StopTime);
}

TEST_F(CommandParserHappyStartTests, IfServiceIdPresentThenExtractedCorrectly) {
  ASSERT_EQ(ServiceIDInput, StartInfo.ServiceID);
}

TEST(CommandParserSadStartTests, ThrowsIfNoJobID) {
  std::string const EmptyJobID;
  auto MessageBuffer = buildRunStartMessage(
      InstrumentNameInput, RunNameInput, NexusStructureInput, EmptyJobID,
      ServiceIDInput, BrokerInput, FilenameInput, StartTimeInput,
      StopTimeInput);

  ASSERT_THROW(FileWriter::CommandParser::extractStartInformation(
                   MessageBuffer.data(), MessageBuffer.size()),
               std::runtime_error)
      << "Expect throw due to empty JobID in run start message";
}

TEST(CommandParserSadStartTests, ThrowsIfNoFilename) {
  std::string const EmptyFilename;
  auto MessageBuffer = buildRunStartMessage(
      InstrumentNameInput, RunNameInput, NexusStructureInput, JobIDInput,
      ServiceIDInput, BrokerInput, EmptyFilename, StartTimeInput,
      StopTimeInput);

  ASSERT_THROW(FileWriter::CommandParser::extractStartInformation(
                   MessageBuffer.data(), MessageBuffer.size()),
               std::runtime_error)
      << "Expect throw due to empty Filename in run start message";
}

TEST(CommandParserSadStartTests, ThrowsIfNoNexusStructure) {
  std::string const EmptyNexusStructure;
  auto MessageBuffer = buildRunStartMessage(
      InstrumentNameInput, RunNameInput, EmptyNexusStructure, JobIDInput,
      ServiceIDInput, BrokerInput, FilenameInput, StartTimeInput,
      StopTimeInput);

  ASSERT_THROW(FileWriter::CommandParser::extractStartInformation(
                   MessageBuffer.data(), MessageBuffer.size()),
               std::runtime_error)
      << "Expect throw due to empty NexusStructure in run start message";
}

TEST(CommandParserSadStartTests, IfStopCommandPassedToStartMethodThenThrows) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_stop",
  "job_id": "qw3rty",
  "service_id": "filewriter1"
})""");

  ASSERT_THROW(FileWriter::CommandParser::extractStartInformation(
                   nlohmann::json::parse(Command)),
               std::runtime_error);
}

TEST(CommandParserSadStartTests, IfNoBrokerThenThrows) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_new",
  "job_id": "qw3rty",
  "file_attributes": {
    "file_name": "a-dummy-name-01.h5"
  },
  "nexus_structure": { }
})""");

  ASSERT_THROW(FileWriter::CommandParser::extractStartInformation(
                   nlohmann::json::parse(Command)),
               std::runtime_error);
}

TEST(CommandParserSadStartTests, IfBrokerIsWrongFormThenThrows) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_new",
  "job_id": "qw3rty",
  "file_attributes": {
    "file_name": "a-dummy-name-01.h5"
  },
  "broker": "1234:somehost",
  "nexus_structure": { }
})""");

  ASSERT_THROW(FileWriter::CommandParser::extractStartInformation(
                   nlohmann::json::parse(Command)),
               std::runtime_error);
}

TEST(CommandParserStartTests, IfNoStartTimeThenUsesSuppliedCurrentTime) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_new",
  "job_id": "qw3rty",
  "broker": "localhost:9092",
  "file_attributes": {
    "file_name": "a-dummy-name-01.h5"
  },
  "nexus_structure": { }
})""");

  auto FakeCurrentTime = std::chrono::milliseconds{987654321};

  auto StartInfo = FileWriter::CommandParser::extractStartInformation(
      nlohmann::json::parse(Command), FakeCurrentTime);

  ASSERT_EQ(FakeCurrentTime, StartInfo.StartTime);
}

TEST(CommandParserStartTests, IfNoStopTimeThenSetToZero) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_new",
  "job_id": "qw3rty",
  "broker": "localhost:9092",
  "file_attributes": {
    "file_name": "a-dummy-name-01.h5"
  },
  "nexus_structure": { }
})""");

  auto StartInfo = FileWriter::CommandParser::extractStartInformation(
      nlohmann::json::parse(Command));

  ASSERT_EQ(std::chrono::milliseconds::zero(), StartInfo.StopTime);
}

TEST(CommandParserStartTests, IfNoServiceIdThenIsBlank) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_new",
  "job_id": "qw3rty",
  "broker": "localhost:9092",
  "file_attributes": {
    "file_name": "a-dummy-name-01.h5"
  },
  "nexus_structure": { }
})""");

  auto StartInfo = FileWriter::CommandParser::extractStartInformation(
      nlohmann::json::parse(Command));

  ASSERT_EQ("", StartInfo.ServiceID);
}

TEST(CommandParserSadStopTests, IfNoJobIdThenThrows) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_stop"
})""");

  ASSERT_THROW(FileWriter::CommandParser::extractStopInformation(
                   nlohmann::json::parse(Command)),
               std::runtime_error);
}

TEST(CommandParserHappyStopTests, IfJobIdPresentThenExtractedCorrectly) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_stop",
  "job_id": "qw3rty"
})""");

  auto StopInfo = FileWriter::CommandParser::extractStopInformation(
      nlohmann::json::parse(Command));

  ASSERT_EQ("qw3rty", StopInfo.JobID);
}

TEST(CommandParserHappyStopTests, IfStopTimePresentThenExtractedCorrectly) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_stop",
  "job_id": "qw3rty",
  "stop_time": 123456790000
})""");

  auto StopInfo = FileWriter::CommandParser::extractStopInformation(
      nlohmann::json::parse(Command));

  ASSERT_EQ(std::chrono::milliseconds{123456790000}, StopInfo.StopTime);
}

TEST(CommandParserStopTests, IfNoStopTimeThenSetToZero) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_stop",
  "job_id": "qw3rty"
})""");

  auto StopInfo = FileWriter::CommandParser::extractStopInformation(
      nlohmann::json::parse(Command));

  ASSERT_EQ(std::chrono::milliseconds::zero(), StopInfo.StopTime);
}

TEST(CommandParserStopTests, IfNoServiceIdThenIsBlank) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_stop",
  "job_id": "qw3rty"
})""");

  auto StopInfo = FileWriter::CommandParser::extractStopInformation(
      nlohmann::json::parse(Command));

  ASSERT_EQ("", StopInfo.ServiceID);
}

TEST(CommandParserStopTests, IfServiceIdPresentThenExtractedCorrectly) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_stop",
  "job_id": "qw3rty",
  "service_id": "filewriter1"
})""");

  auto StopInfo = FileWriter::CommandParser::extractStopInformation(
      nlohmann::json::parse(Command));

  ASSERT_EQ("filewriter1", StopInfo.ServiceID);
}

TEST(CommandParserSadStopTests, IfStartCommandPassedToStopMethodThenThrows) {
  std::string Command(R"""(
{
  "cmd": "FileWriter_new",
  "job_id": "qw3rty",
  "file_attributes": {
    "file_name": "a-dummy-name-01.h5"
  },
  "nexus_structure": { }
})""");

  ASSERT_THROW(FileWriter::CommandParser::extractStopInformation(
                   nlohmann::json::parse(Command)),
               std::runtime_error);
}
