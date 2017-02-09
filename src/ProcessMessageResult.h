#pragma once

namespace BrightnESS {
namespace FileWriter {


/// %Result of a call to `process_message`.
/// Can be extended later for more detailed reporting.
/// Streamer currently does not accept the return value, therefore currently
/// not used.
class ProcessMessageResult {
public:
static ProcessMessageResult OK(int64_t ts);
static ProcessMessageResult ERR();
static ProcessMessageResult ALL_SOURCES_FULL();
inline bool is_OK() { return _ts >= 0; }
inline bool is_ERR() { return _ts == -1; }
inline bool is_ALL_SOURCES_FULL() { return _ts == -2; }
inline int64_t ts() { return _ts; }
private:
int64_t _ts = -1;
};

}
}