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
            try
            {
                streamSocket_->shutdown(LIBSOCKET_READ | LIBSOCKET_WRITE);
            }
            catch (const libsocket::socket_exception& e)
            {
                // Swallow this exception:  If the client closed first, this operation will be
                // incomplete, but if we are shutting down first, we need to do it.
            }
        }

        //
        // The main operation.  If this socket becomes readable, call this method.
        // We read the query from the command and control client and process it.
        // If the read is empty, assume the socket was closed by the client.
        // Return true unless the client closed the socket and we need to clean up this end.
        //
        bool HandleInput(unique_ptr<IQueryHandler> const & queryHandler)
        {
            try
            {
                string queryFragment;
                queryFragment.resize(1000);
                *streamSocket_ >> queryFragment;

                if (queryFragment.empty()) return false;

                BuildInputString(queryFragment);
                startTime_ = high_resolution_clock::now();

                cout << "Query: " << query_ << "\n";

                string response = queryHandler->HandleQuery(query_);
                BuildAndSendResponse(response);

                //*streamSocket_ << response;

                return true;
            }
            catch(const libsocket::socket_exception& e)
            {
                return false;
            }
        }

        void DoPeriodicSupport(unique_ptr<IQueryHandler> const & queryHandler)
        {
            if (firstPeriodicSupport)
            {
                startTime_ = high_resolution_clock::now();
                firstPeriodicSupport = false;
                return;
            }

            const nanoseconds period = nanoseconds(250ms);

            embeddedpenguins::core::neuron::model::time_point currentTime = high_resolution_clock::now();
            if (currentTime - startTime_ > period)
            {
                startTime_ += period;

                json query;
                query["query"] = "fullstatus";
                string response = "  " + queryHandler->HandleQuery(query.dump());
                BuildAndSendResponse(response);
            }
        }

    private:
        void BuildAndSendResponse(string& response)
        {
            //cout << "Sending full status: " << response << "\n";
            try
            {
                unsigned short bufferLength = response.length();

                string responseWithLength = "  " + response;
                auto* c_response = const_cast<char*>(responseWithLength.c_str());

                *c_response = *(char *)&bufferLength;
                *(c_response + 1) = *((char *)&bufferLength + 1);

                streamSocket_->snd(c_response, responseWithLength.length());
            }
            catch(const std::exception& e)
            {
                // Swallow exception:  In the rare case that the client closed
                // their end just before we send, this will fail.
            }
        }

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
