#pragma once

#include <iostream>
#include <string>
#include <memory>

#include "libsocket/exception.hpp"
#include "libsocket/inetserverstream.hpp"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::string;
    using std::unique_ptr;

    using libsocket::inet_stream;
    using libsocket::inet_stream_server;

    class CommandControlSocket
    {
        unique_ptr<inet_stream> streamSocket_;

    public:
        inet_stream* StreamSocket() const { return streamSocket_.get(); }

    public:
        CommandControlSocket(inet_stream_server* ccSocket) :
            streamSocket_(ccSocket->accept2())
        {
            // An attempt to stop the re-use timer at the system level, so we can restart immediately.
            int reuseaddr = 1;
            streamSocket_->set_sock_opt(streamSocket_->getfd(), SO_REUSEADDR, (const char*)(&reuseaddr), sizeof(int));

            cout << "CommandControlSocket main ctor\n";
        }

        CommandControlSocket(const CommandControlSocket& other) = delete;
        CommandControlSocket& operator=(const CommandControlSocket& other) = delete;
        CommandControlSocket(CommandControlSocket&& other) noexcept = delete;
        CommandControlSocket& operator=(CommandControlSocket&& other) noexcept = delete;

        virtual ~CommandControlSocket()
        {
            cout << "CommandControlSocket dtor \n";
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
            // TODO handle actual queries.
            string query;
            query.resize(100);
            *streamSocket_ >> query;

            if (query.empty()) return false;

            cout << "Query: " << query;

            *streamSocket_ << "Hello " << query << "\n";

            return true;
        }
    };
}
