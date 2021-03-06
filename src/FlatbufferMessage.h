// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "logger.h"

namespace FileWriter {
class BufferTooSmallError : public std::runtime_error {
public:
  explicit BufferTooSmallError(const std::string &what)
      : std::runtime_error(what){};
};

class UnknownFlatbufferID : public std::runtime_error {
public:
  explicit UnknownFlatbufferID(const std::string &what)
      : std::runtime_error(what){};
};

class NotValidFlatbuffer : public std::runtime_error {
public:
  explicit NotValidFlatbuffer(const std::string &what)
      : std::runtime_error(what){};
};

/// \brief A wrapper around a databuffer which holds a flatbuffer.
///
/// Used to simplify passing around flatbuffers and the most important pieces of
/// meta-data.
///
/// \note Does not take ownership of any pointers. You must make sure to
/// free the pointers you passed yourself.
class FlatbufferMessage {
public:
  using SrcHash = size_t;

  /// Constructor is used in unit testing code to simplify set-up.
  FlatbufferMessage() = default;

  /// \brief Verifies the data in the flatbuffer to make sure if it is valid.
  ///
  /// \param BufferPtr Pointer to memory location containing flatbuffer.
  /// \param Size Size of flatbuffer in bytes.
  FlatbufferMessage(char const *BufferPtr, size_t Size);

  /// Default destructor.
  ~FlatbufferMessage() = default;

  /// \brief Returns the state of the FlatbufferMessage.
  ///
  /// \return `true` if valid, `false` if not.
  bool isValid() const { return Valid; };

  /// \brief Get the source name of the flatbuffer.
  ///
  /// Extracted using FileWriter::FlatbufferReader::source_name().
  ///
  /// \return The source name if flatbuffer is valid, an empty string if it is
  /// not.
  std::string getSourceName() const { return Sourcename; };

  /// \brief Get the timestamp of the flatbuffer.
  ///
  /// Extracted using FileWriter::FlatbufferReader::timestamp().
  ///
  /// \return The timestamp if flatbuffer is valid, 0 if it is not.
  std::uint64_t getTimestamp() const { return Timestamp; };

  /// \brief Get the hash from a combination of the flatbuffer type and source
  /// name.
  ///
  /// \return The std::hash<std::string> from flatbuffer id + source name.
  /// Returns 0 if flatbuffer is invalid.
  SrcHash getSourceHash() const { return SourceNameIDHash; };

  /// \brief Get flatbuffer ID.
  ///
  /// \return Returns the four character flatbuffer ID or empty string if
  /// invalid.
  std::string getFlatbufferID() const { return ID; };

  /// \brief Get pointer to flatbuffer.
  ///
  /// \return Pointer to flatbuffer data if flatbuffer is valid, `nullptr` if it
  /// is not.
  char const *data() const { return DataPtr; };

  /// \brief Get size of flatbuffer.
  ///
  /// \return Size of flatbuffer in bytes if flatbuffer is valid, 0 if it is
  /// not.
  size_t size() const { return DataSize; };

private:
  void extractPacketInfo();
  char const *const DataPtr{nullptr};
  size_t const DataSize{0};
  SrcHash SourceNameIDHash{0};
  std::string Sourcename;
  std::string ID;
  std::uint64_t Timestamp{0};
  bool Valid{false};
};

FlatbufferMessage::SrcHash calcSourceHash(std::string const &ID,
                                          std::string const &Name);
} // namespace FileWriter
