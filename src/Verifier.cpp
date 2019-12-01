#include <iostream>
#include <fstream>

#include <librdkafka/rdkafkacpp.h>

#include "ns10_cache_entry_generated.h"

enum class Order { TtTVK, TtVTK, TVKTt, VKTtT };

bool verifyBuffer(const char* RawData, size_t Size) {
  auto Verifier = flatbuffers::Verifier(
     reinterpret_cast<const uint8_t *>(RawData), Size);
  return VerifyCacheEntryBuffer(Verifier);
}

void verifyDiskBuffer(const std::string& FileName) {
  std::ifstream InFile(FileName, std::ios::binary);
  if (!InFile) {
    std::cout << "Error: can't open file " << FileName << "\n";
    return;
  }
  InFile.seekg(0, InFile.end);
  auto FileSize = InFile.tellg();
  auto RawData = std::make_unique<char[]>(FileSize);
  InFile.seekg(0, InFile.beg);
  InFile.read(RawData.get(), FileSize);

  bool Valid = verifyBuffer(RawData.get(), FileSize);
  std::cout << "File " << FileName << " valid: " << Valid << "\n";

}

std::shared_ptr<RdKafka::KafkaConsumer> createConsumer(const std::string& Broker) {
 std::string ErrStr;

 auto conf = std::unique_ptr<RdKafka::Conf>(
     RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
 conf->set("metadata.broker.list", Broker, ErrStr);

 auto KafkaConsumer =
     std::unique_ptr<RdKafka::KafkaConsumer>(RdKafka::KafkaConsumer::create(
         conf.get(), ErrStr));
 if (!ErrStr.empty()) {
   std:: cout << "Error creating KafkaConsumer in Consumer::Consumer.\n";
 }
  return KafkaConsumer;
}

struct MessageMetadataStruct {
  std::int64_t Offset;
  int64_t Timestamp;
  int32_t Partition;
  std::string Payload;
  bool PartitionEOF = false;
  std::string TimestampISO;
  std::string TimestampISO8601;
  std::string Key;
  bool KeyPresent = false;
};

MessageMetadataStruct consumeMessage(std::shared_ptr<RdKafka::KafkaConsumer> KafkaConsumer) {
  using RdKafka::Message;
  MessageMetadataStruct DataToReturn;

  auto KafkaMsg = std::unique_ptr<Message>(KafkaConsumer->consume(1000));
  switch (KafkaMsg->err()) {
  case RdKafka::ERR_NO_ERROR:
    // Real message
    if (KafkaMsg->len() > 0) {
      std::string Payload(static_cast<const char *>(KafkaMsg->payload()),
                          static_cast<int>(KafkaMsg->len()));
      DataToReturn.Payload = Payload;
      DataToReturn.Partition = KafkaMsg->partition();
      DataToReturn.Offset = KafkaMsg->offset();
      DataToReturn.Timestamp = KafkaMsg->timestamp().timestamp;
      if (KafkaMsg->key_len() != 0) {
        DataToReturn.Key = *KafkaMsg->key();
        DataToReturn.KeyPresent = true;
      }
    } else {
      // If RdKafka indicates no error then we should always get a
      // non-zero length message
      throw std::runtime_error("KafkaTopicSubscriber::consumeMessage() - Kafka "
                               "indicated no error but a zero-length payload "
                               "was received");
    }
    break;
  case RdKafka::ERR__TIMED_OUT:
    throw std::runtime_error(
                           "KafkaTopicSubscriber::consumeMessage() - "+
                    RdKafka::err2str(KafkaMsg->err()));
  case RdKafka::ERR__PARTITION_EOF:
    DataToReturn.PartitionEOF = true;
    // Not errors as the broker might come back or more data might be pushed
    break;
  default:
    /* All other errors */
    throw std::runtime_error(
                             "KafkaTopicSubscriber::consumeMessage() - "+
                    RdKafka::err2str(KafkaMsg->err()));
  }
  return DataToReturn;
}

void verifyKafkaBuffers(int argc, char **argv) {
  if(argc < 4) {
    std::cout << "Invalid number of arguments\n";
  }
  std::cout << std::string(argv[2]) << "\n";
  auto KafkaConsumer = createConsumer(std::string(argv[2]));
  
}



int main(int argc, char **argv) {
  if (argc < 1){
    std::cout << "Error: argument required\n";
    return -1;
  }

  if (std::string(argv[1]) == std::string("--broker")) {
    if (argc < 3) {
      std::cout << "Error: filename required\n";
      return -1;
    }
    verifyKafkaBuffers(argc, argv);
  }
  if (std::string(argv[1]) == std::string("--validate")) {
    if (argc < 3) {
      std::cout << "Error: filename required\n";
      return -1;
    }
    verifyDiskBuffer(std::string(argv[2]));
  }
  return 0;
}
