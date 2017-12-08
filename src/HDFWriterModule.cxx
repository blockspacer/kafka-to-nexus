#include "HDFWriterModule.h"

namespace FileWriter {

namespace HDFWriterModuleRegistry {

std::map<HDFWriterModuleRegistry::Key, HDFWriterModuleRegistry::Value> &
items() {
  static std::map<HDFWriterModuleRegistry::Key, HDFWriterModuleRegistry::Value>
      _items;
  return _items;
}

HDFWriterModuleRegistry::Value &find(HDFWriterModuleRegistry::Key const &key) {
  static HDFWriterModuleRegistry::Value empty;
  auto &_items = items();
  auto f = _items.find(key);
  if (f == _items.end()) {
    return empty;
  }
  return f->second;
}

void registrate(Key key, Value value) {
  auto &m = items();
  if (m.find(key) != m.end()) {
    auto s = fmt::format("ERROR entry for key [{}] exists already", key);
    throw std::runtime_error(s);
  }
  m[key] = std::move(value);
}
}

namespace HDFWriterModule_detail {

static std::map<int8_t, std::string> const g_InitResult_strings{
    {0, "OK"}, {-1, "ERROR_IO"}, {-2, "ERROR_INCOMPLETE_CONFIGURATION"},
};

std::string InitResult::to_str() const {
  auto const &m = g_InitResult_strings;
  auto const it = m.find(v);
  if (it == m.end()) {
    return "ERROR_UNKNOWN_VALUE";
  }
  return it->second;
}

static std::map<int8_t, std::string> const g_WriteResult_strings{
    {0, "OK"},
    {-1, "ERROR_IO"},
    {-2, "ERROR_BAD_FLATBUFFER"},
    {-3, "ERROR_DATA_STRUCTURE_MISMATCH"},
    {-4, "ERROR_DATA_TYPE_MISMATCH"},
};

std::string WriteResult::to_str() const {
  auto const &m = g_WriteResult_strings;
  auto const it = m.find(v);
  if (it == m.end()) {
    return "ERROR_UNKNOWN_VALUE";
  }
  return it->second;
}
}
}