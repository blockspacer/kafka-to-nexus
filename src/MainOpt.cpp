// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "MainOpt.h"
#include "URI.h"
#include "helper.h"
#include "json.h"
#include <iostream>

using uri::URI;

// For reasons unknown, the presence of the constructor caused the integration
// test to fail, with the NeXus file being created, but no data written to it.
// While the cause of this problem is not discovered and fixed, use the
// following init function.
void MainOpt::init() {
  ServiceID = fmt::format("kafka-to-nexus--host:{}--pid:{}",
                          gethostname_wrapper(), getpid_wrapper());
}

int MainOpt::parseJsonCommands() {
  auto jsontxt = readFileIntoVector(CommandsJsonFilename);
  using nlohmann::json;
  try {
    CommandsJson = json::parse(jsontxt);
  } catch (...) {
    return 1;
  }
  findAndAddCommands();
  return 0;
}

void MainOpt::findAndAddCommands() {
  if (auto v = find<nlohmann::json>("commands", CommandsJson)) {
    for (auto const &Command : *v) {
      // cppcheck-suppress useStlAlgorithm
      CommandsFromJson.emplace_back(Command.dump());
    }
  }
}

void setupLoggerFromOptions(MainOpt const &opt) {
  setUpLogging(opt.LoggingLevel, opt.ServiceID, opt.LogFilename,
               opt.GraylogLoggerAddress);
}
