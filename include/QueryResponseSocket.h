#pragma once

#include <iostream>
#include <string>
#include <memory>

#include "libsocket/exception.hpp"
#include "libsocket/inetserverstream.hpp"

#include "IQueryHandler.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::string;
    using std::unique_ptr;

    using libsocket::inet_stream;
    using libsocket::inet_stream_server;

    class QueryResponseSocket
    {
        unique_ptr<IQueryHandler> queryHandler_ { };
        unique_ptr<inet_stream> streamSocket_;

        string query_ { };

    public:
        inet_stream* StreamSocket() const { return streamSocket_.get(); }

    public:
        QueryResponseSocket(inet_stream_server* ccSocket, unique_ptr<IQueryHandler> queryHandler) :
            queryHandler_(std::move(queryHandler)),
            streamSocket_(ccSocket->accept2())
        {
            // An attempt to stop the re-use timer at the system level, so we can restart immediately.
            int reuseaddr = 1;
            streamSocket_->set_sock_opt(streamSocket_->getfd(), SO_REUSEADDR, (const char*)(&reuseaddr), sizeof(int));

            cout << "QueryResponseSocket main ctor\n";
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
        bool HandleInput()
        {
            string queryFragment;
            queryFragment.resize(1000);
            *streamSocket_ >> queryFragment;

            if (queryFragment.empty()) return false;

            BuildInputString(queryFragment);

            cout << "Query: " << query_;

            const string& response = queryHandler_->HandleQuery(query_);

            *streamSocket_ << response << "\n";

            return true;
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
