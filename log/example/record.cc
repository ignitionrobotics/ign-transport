/*
 * Copyright (C) 2018 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

/// \brief Example of recording all ignition transport topics.
/// This will record all topics and currently published to a file.
/// Launch the ignition-transport publisher example so this example has
/// something to record.

#include <chrono>
#include <cstdint>
#include <iostream>
#include <regex>
#include <thread>
#include <ignition/transport/log/Recorder.hh>

using namespace std::chrono_literals;

//////////////////////////////////////////////////
int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " OUTPUT.tlog\n";
    return -1;
  }

  ignition::transport::log::Recorder recorder;

  // Record all topics
  recorder.AddTopic(std::regex(".*"));

  // Begin recording, saving received messages to the given file
  auto result = recorder.Start(argv[1]);
  if (ignition::transport::log::RecorderError::SUCCESS != result)
  {
    std::cerr << "Failed to start recording: " << static_cast<int64_t>(result)
              << "\n";
    return -1;
  }

  std::cout << "Recording for 30 seconds\n";
  std::this_thread::sleep_for(30s);
  recorder.Stop();

  return 0;
}
