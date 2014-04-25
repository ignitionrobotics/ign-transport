/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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

#include <google/protobuf/message.h>
#include <iostream>
#include <mutex>
#include <string>
#include <zmq.hpp>
#include "ignition/transport/Node.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/zmsg.hpp"

using namespace ignition;

//////////////////////////////////////////////////
transport::Node::Node(bool _verbose)
  : dataPtr(transport::NodePrivate::GetInstance(_verbose))
{
}

//////////////////////////////////////////////////
transport::Node::~Node()
{
}

//////////////////////////////////////////////////
int transport::Node::Advertise(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr.mutex);

  this->dataPtr.topics.SetAdvertisedByMe(_topic, true);

  for (auto addr : this->dataPtr.myAddresses)
    this->dataPtr.SendAdvertiseMsg(transport::AdvType, _topic, addr);

  return 0;
}

//////////////////////////////////////////////////
int transport::Node::UnAdvertise(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr.mutex);

  this->dataPtr.topics.SetAdvertisedByMe(_topic, false);

  return 0;
}

//////////////////////////////////////////////////
int transport::Node::Publish(const std::string &_topic,
            const google::protobuf::Message &_message)
{
  assert(_topic != "");

  std::string data;
  _message.SerializeToString(&data);

  return this->dataPtr.Publish(_topic, data);
}

//////////////////////////////////////////////////
int transport::Node::Subscribe(const std::string &_topic,
  void(*_cb)(const std::string &, const std::string &))
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr.mutex);

  if (this->dataPtr.verbose)
    std::cout << "\nSubscribe (" << _topic << ")\n";

  // Register our interest on the topic
  // The last subscribe call replaces previous subscriptions. If this is
  // a problem, we have to store a list of callbacks.
  this->dataPtr.topics.SetSubscribed(_topic, true);
  this->dataPtr.topics.SetCallback(_topic, _cb);

  // Discover the list of nodes that publish on the topic
  return this->dataPtr.SendSubscribeMsg(transport::SubType, _topic);
}

//////////////////////////////////////////////////
int transport::Node::UnSubscribe(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr.mutex);

  if (this->dataPtr.verbose)
    std::cout << "\nUnubscribe (" << _topic << ")\n";

  this->dataPtr.topics.SetSubscribed(_topic, false);
  this->dataPtr.topics.SetCallback(_topic, nullptr);

  // Remove the filter for this topic
  this->dataPtr.subscriber->setsockopt(ZMQ_UNSUBSCRIBE, _topic.data(),
                                       _topic.size());
  return 0;
}
