/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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

#ifndef IGN_TRANSPORT_NODESHAREDPRIVATE_HH_
#define IGN_TRANSPORT_NODESHAREDPRIVATE_HH_

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <zmq.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <atomic>
#include <memory>
#include <queue>
#include <vector>

#include "ignition/transport/Discovery.hh"

namespace ignition
{
  namespace transport
  {
    // Inline bracket to help doxygen filtering.
    inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {
    struct PublicationMetadata
    {
      uint64_t stamp = 0;
      uint64_t seq = 0;
    };

    class Statistics
    {
      public: void Update(double _stat)
      {
        this->count++;
        const double currentAvg = this->avgPeriodMilliseconds;
        this->avgPeriodMilliseconds = currentAvg +
          (_stat - currentAvg) / this->count;
        this->min = std::min(this->min, _stat);
        this->max = std::max(this->max, _stat);
        this->sumSquareMeanDist += (_stat - currentAvg) *
          (_stat - this->avgPeriodMilliseconds);
      }

      /// \brief Get the hertz rate.
      public: double Hz()
      {
        if (this->count >= 1)
          return 1.0 / (this->avgPeriodMilliseconds/1000.0);
        else
          return std::nan("");
      }

      public: double StdDev()
      {
        return std::sqrt(this->sumSquareMeanDist / this->count);
      }

      public: uint64_t count = 0;
      public: double avgPeriodMilliseconds = 0;
      public: double sumSquareMeanDist = 0;
      public: double min = std::numeric_limits<double>::max();
      public: double max = std::numeric_limits<double>::min();
    };

    class TopicStatistics
    {
      public: void Update(const std::string &_sender,
                          const PublicationMetadata &_meta)
      {
        uint64_t now =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now().time_since_epoch()).count();

        if (this->prevPublicationStamp != 0)
        {
          this->publication.Update(_meta.stamp - this->prevPublicationStamp);
          this->receive.Update(now - this->prevReceiveStamp);

          if (this->seq[_sender] + 1 != _meta.seq)
          {
            // std::cout << "Dropped\n";
            this->droppedMsgCount++;
          }
        }

        this->prevPublicationStamp = _meta.stamp;
        this->prevReceiveStamp = now;

        this->seq[_sender] = _meta.seq;
      }


      public: std::map<std::string, uint64_t> seq;
      public: Statistics publication;
      public: Statistics receive;
      public: uint64_t droppedMsgCount = 0;
      public: uint64_t prevPublicationStamp = 0;
      public: uint64_t prevReceiveStamp = 0;
    };

    //
    // Private data class for NodeShared.
    class NodeSharedPrivate
    {
      // Constructor
      public: NodeSharedPrivate() :
                context(new zmq::context_t(1)),
                publisher(new zmq::socket_t(*context, ZMQ_PUB)),
                subscriber(new zmq::socket_t(*context, ZMQ_SUB)),
                requester(new zmq::socket_t(*context, ZMQ_ROUTER)),
                responseReceiver(new zmq::socket_t(*context, ZMQ_ROUTER)),
                replier(new zmq::socket_t(*context, ZMQ_ROUTER))
      {
      }

      /// \brief Initialize security
      public: void SecurityInit();

      /// \brief Handle new secure connections
      public: void SecurityOnNewConnection();

      /// \brief Access control handler for plain security.
      /// This function is designed to be run in a thread.
      public: void AccessControlHandler();

      //////////////////////////////////////////////////
      ///////    Declare here the ZMQ Context    ///////
      //////////////////////////////////////////////////

      /// \brief 0MQ context. Always declare this object before any ZMQ socket
      /// to make sure that the context is destroyed after all sockets.
      public: std::unique_ptr<zmq::context_t> context;

      //////////////////////////////////////////////////
      ///////     Declare here all ZMQ sockets   ///////
      //////////////////////////////////////////////////

      /// \brief ZMQ socket to send topic updates.
      public: std::unique_ptr<zmq::socket_t> publisher;

      /// \brief ZMQ socket to receive topic updates.
      public: std::unique_ptr<zmq::socket_t> subscriber;

      /// \brief ZMQ socket for sending service call requests.
      public: std::unique_ptr<zmq::socket_t> requester;

      /// \brief ZMQ socket for receiving service call responses.
      public: std::unique_ptr<zmq::socket_t> responseReceiver;

      /// \brief ZMQ socket to receive service call requests.
      public: std::unique_ptr<zmq::socket_t> replier;

      /// \brief Thread the handle access control
      public: std::thread accessControlThread;

      //////////////////////////////////////////////////
      /////// Declare here the discovery object  ///////
      //////////////////////////////////////////////////

      /// \brief Discovery service (messages).
      public: std::unique_ptr<MsgDiscovery> msgDiscovery;

      /// \brief Discovery service (services).
      public: std::unique_ptr<SrvDiscovery> srvDiscovery;

      //////////////////////////////////////////////////
      /////// Other private member variables     ///////
      //////////////////////////////////////////////////

      /// \brief When true, the reception thread will finish.
      public: std::atomic<bool> exit = false;

      /// \brief Timeout used for receiving messages (ms.).
      public: static const int Timeout = 250;

      ////////////////////////////////////////////////////////////////
      /////// The following is for asynchronous publication of ///////
      /////// messages to local subscribers.                    ///////
      ////////////////////////////////////////////////////////////////

      /// \brief Encapsulates information needed to publish a message. An
      /// instance of this class is pushed onto a publish queue, pubQueue, when
      /// a message is published through Node::Publisher::Publish.
      /// The pubThread processes the pubQueue in the
      /// NodeSharedPrivate::PublishThread function.
      ///
      /// A producer-consumer mechanism is used to send messages so that
      /// Node::Publisher::Publish function does not block while executing
      /// local subscriber callbacks.
      public: struct PublishMsgDetails
              {
                /// \brief All the local subscription handlers.
                public: std::vector<ISubscriptionHandlerPtr> localHandlers;

                /// \brief All the raw handlers.
                public: std::vector<RawSubscriptionHandlerPtr> rawHandlers;

                /// \brief Buffer for the raw handlers.
                public: std::unique_ptr<char[]> sharedBuffer = nullptr;

                /// \brief Msg copy for the local handlers.
                public: std::unique_ptr<ProtoMsg> msgCopy = nullptr;

                /// \brief Message size.
                // cppcheck-suppress unusedStructMember
                public: std::size_t msgSize = 0;

                /// \brief Information about the topic and type.
                public: MessageInfo info;
              };

      /// \brief Publish thread used to process the pubQueue.
      public: std::thread pubThread;

      /// \brief Mutex to protect the pubThread and pubQueue.
      public: std::mutex pubThreadMutex;

      /// \brief Queue onto which new messages are pushed. The pubThread
      /// will pop off the messages and send them to local subscribers.
      public: std::queue<std::unique_ptr<PublishMsgDetails>> pubQueue;

      /// \brief used to signal when new work is available
      public: std::condition_variable signalNewPub;

      /// \brief Handles local publication of messages on the pubQueue.
      public: void PublishThread();

      /// \brief Topic publication sequence numbers.
      public: std::map<std::string, uint64_t> topicPubSeq;

      /// \brief Statistics for a topic. The key in the map is the topic
      /// name and the value contains the topic statistics.
      public: std::map<std::string, TopicStatistics> topicStats;

      /// \brief Set of topics that have statistics enabled.
      public: std::set<std::string> enabledTopicStatistics;
    };
    }
  }
}
#endif
