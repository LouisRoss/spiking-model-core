#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <map>
#include <utility>
#include <string>
#include <algorithm>

#include "libsocket/exception.hpp"
#include "libsocket/inetserverstream.hpp"
#include "libsocket/select.hpp"
#include "libsocket/socket.hpp"


#include "ICommandControlAcceptor.h"
#include "CommandControlSocket.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::begin;
    using std::end;
    using std::vector;
    using std::map;
    using std::string;
    using std::unique_ptr;
    using std::make_unique;

    using libsocket::socket;
    using libsocket::inet_stream;
    using libsocket::inet_stream_server;
    using libsocket::selectset;

    class CommandControlListenSocket : public ICommandControlAcceptor
    {
        inet_stream_server server_;
        unique_ptr<selectset<socket>> selectSet_ { };

        map<socket*, unique_ptr<CommandControlSocket>> ccSockets_ { };

    public:
        CommandControlListenSocket(const string& host, const string& port) :
            server_(host, port, LIBSOCKET_IPv4)
        {
            // An attempt to stop the re-use timer at the system level, so we can restart immediately.
            int reuseaddr = 1;
            server_.set_sock_opt(server_.getfd(), SO_REUSEADDR, (const char*)(&reuseaddr), sizeof(int));
        }

        CommandControlListenSocket(const CommandControlListenSocket& other) = delete;
        CommandControlListenSocket& operator=(const CommandControlListenSocket& other) = delete;
        CommandControlListenSocket(CommandControlListenSocket&& other) noexcept = delete;
        CommandControlListenSocket& operator=(CommandControlListenSocket&& other) noexcept = delete;

        virtual ~CommandControlListenSocket()
        {
            cout << "CommandControlListenSocket dtor\n";
            server_.destroy();
        }

        virtual bool Initialize() override
        {
            MakeSelectSet();
            return true;
        }

        //
        // The main action handler.  Call periodically from the main loop.
        // Wait a few milliseconds for one of the sockets in the select set
        // to be readable, then handle all that are.
        // Use dynamic_cast to distinguish between socket types.
        //
        virtual void AcceptAndExecute() override
        {
            auto [readSockets, _] = selectSet_->wait(10'000);

            for (auto readSocket : readSockets)
            {
                inet_stream_server* listenSocket = dynamic_cast<inet_stream_server*>(readSocket);
                if (listenSocket != nullptr)
                    AcceptNewConnection(readSocket, listenSocket);

                inet_stream* dataSocket = dynamic_cast<inet_stream*>(readSocket);
                if (dataSocket != nullptr)
                    HandleInput(readSocket, dataSocket);
            }
        }

    private:
        void AcceptNewConnection(socket* readSocket, inet_stream_server* listenSocket)
        {
            if (ccSockets_.find(readSocket) == end(ccSockets_))
            {
                cout << "CommandControlListenSocket found readable socket is listen socket, creating new connection\n";
                auto dataSocket = make_unique<CommandControlSocket>(listenSocket);
                ccSockets_[dataSocket->StreamSocket()] = std::move(dataSocket);
                MakeSelectSet();
            }
        }

        void HandleInput(socket* readSocket, inet_stream* dataSocket)
        {
            cout << "CommandControlListenSocket found readable data socket, handling request\n";
            auto iSocket = ccSockets_.find(readSocket);
            if (iSocket != end(ccSockets_))
            {
                if (!iSocket->second->HandleInput())
                {
                    cout << "CommandControlListenSocket: readable data socket closed by client\n";
                    ccSockets_.erase(iSocket);
                    MakeSelectSet();
                }
            }
        }

        //
        // Throw away the existing select set (if any) and create a new one
        // with read function for the listen server socket and the streaming
        // sockets of all current data sockets.
        //
        void MakeSelectSet()
        {
            selectSet_.reset(new selectset<socket>());

            selectSet_->add_fd(server_, LIBSOCKET_READ);
            for (auto iSocket = begin(ccSockets_); iSocket != end(ccSockets_); iSocket++)
            {
                selectSet_->add_fd(*iSocket->second->StreamSocket(), LIBSOCKET_READ);
            }
        }
    };
}
