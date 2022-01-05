#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <chrono>

#include "libsocket/exception.hpp"
#include "libsocket/inetserverstream.hpp"

#include "IQueryHandler.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::string;
    using std::unique_ptr;
    using std::function;
    using std::chrono::milliseconds;
    using std::chrono::duration_cast;
    using namespace std::chrono_literals;

    using libsocket::inet_stream;
    using libsocket::inet_stream_server;

    class QueryResponseSocket
    {
        unique_ptr<inet_stream> streamSocket_;

        string query_ { };
        embeddedpenguins::core::neuron::model::time_point startTime_ {};
        bool firstPeriodicSupport { true };


    public:
        inet_stream* StreamSocket() const { return streamSocket_.get(); }

    public:
        QueryResponseSocket(inet_stream_server* ccSocket) :
            streamSocket_(ccSocket->accept2())
        {
            // An attempt to stop the re-use timer at the system level, so we can restart immediately.
            int reuseaddr = 1;
            streamSocket_->set_sock_opt(streamSocket_->getfd(), SO_REUSEADDR, (const char*)(&reuseaddr), sizeof(int));

            cout << "QueryResponseSocket main ctor with host " << streamSocket_->gethost() << " and port " << streamSocket_->getport() << "\n";
        }

        QueryResponseSocket(const QueryResponseSocket& other) = delete;
        QueryResponseSocket& operator=(const QueryResponseSocket& other) = delete;
        QueryResponseSocket(QueryResponseSocket&& other) noexcept = delete;
        QueryResponseSocket& operator=(QueryResponseSocket&& other) noexcept = delete;

        virtual ~QueryResponseSocket()
        {
            cout << "QueryResponseSocket dtor \n";
            streamSocket_->shutdown(LIBSOCKET_READ | LIBSOCKET_WRITE);
        }

        //
        // The main operation.  If this socket becomes readable, call this method.
        // We read the query from the command and control client and process it.
        // If the read is empty, assume the socket was closed by the client.
        // Return true unless the client closed the socket and we need to clean up this end.
        //
        bool HandleInput(unique_ptr<IQueryHandler> const & queryHandler)
        {
            string queryFragment;
            queryFragment.resize(1000);
            *streamSocket_ >> queryFragment;

            if (queryFragment.empty()) return false;

            BuildInputString(queryFragment);
            startTime_ = high_resolution_clock::now();

            cout << "Query: " << query_;

            const string& response = queryHandler->HandleQuery(query_);

            *streamSocket_ << response;

            return true;
        }

        void DoPeriodicSupport(unique_ptr<IQueryHandler> const & queryHandler)
        {
            if (firstPeriodicSupport)
            {
                startTime_ = high_resolution_clock::now();
                firstPeriodicSupport = false;
                return;
            }

            const nanoseconds period = nanoseconds(1000ms);

            embeddedpenguins::core::neuron::model::time_point currentTime = high_resolution_clock::now();
            if (currentTime - startTime_ > period)
            {
                startTime_ += period;

                json query;
                query["query"] = "fullstatus";
                const string& response = queryHandler->HandleQuery(query.dump());

                *streamSocket_ << response;
            }

        }

    private:
        void BuildInputString(string& queryFragment)
        {
            query_.clear();

            do
            {
                query_ += queryFragment;

                if (queryFragment.size() == 1000)
                {
                    queryFragment.clear();
                    queryFragment.resize(1000);
                    *streamSocket_ >> queryFragment;

                    if (!queryFragment.empty()) query_ += queryFragment;
                }
            } while (queryFragment.size() == 1000);
        }
    };
}
