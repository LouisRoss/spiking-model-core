#pragma once

#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <utility>
#include <functional>

#include "libsocket/exception.hpp"
#include "libsocket/inetserverstream.hpp"
#include "libsocket/select.hpp"
#include "libsocket/socket.hpp"


#include "ICommandControlAcceptor.h"
#include "QueryResponseSocket.h"
#include "IQueryHandler.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::map;
    using std::string;
    using std::unique_ptr;
    using std::make_unique;
    using std::begin;
    using std::end;
    using std::function;

    using libsocket::socket;
    using libsocket::inet_stream;
    using libsocket::inet_stream_server;
    using libsocket::selectset;

    class QueryResponseListenSocket : public ICommandControlAcceptor
    {
        inet_stream_server server_;
        unique_ptr<selectset<socket>> selectSet_ { };

        map<socket*, unique_ptr<QueryResponseSocket>> ccSockets_ { };

    public:
        QueryResponseListenSocket(const string& host, const string& port) :
            server_(host, port, LIBSOCKET_IPv4)
        {
            // Louis Ross - I modified libsocket to always apply SO_REUSEADDR=1 just before a server socket binds.
            cout << "QueryResponseListenSocket with fd=" << server_.getfd() << " listening at " << host << ":" << port << "\n";
        }

        QueryResponseListenSocket(const QueryResponseListenSocket& other) = delete;
        QueryResponseListenSocket& operator=(const QueryResponseListenSocket& other) = delete;
        QueryResponseListenSocket(QueryResponseListenSocket&& other) noexcept = delete;
        QueryResponseListenSocket& operator=(QueryResponseListenSocket&& other) noexcept = delete;

        virtual ~QueryResponseListenSocket()
        {
            cout << "QueryResponseListenSocket dtor\n";
            server_.destroy();
        }

        virtual const string& Description() override
        {
            static string description { "Listen Socket" };
            return description;
        }

        virtual bool ParseArguments(int argc, char* argv[]) override
        {
            // No arguments used in this acceptor.
            return true;
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
        virtual bool AcceptAndExecute(unique_ptr<IQueryHandler> const & queryHandler) override
        {
            auto [readSockets, _] = selectSet_->wait(10'000);

            for (auto readSocket : readSockets)
            {
                inet_stream_server* listenSocket = dynamic_cast<inet_stream_server*>(readSocket);
                if (listenSocket != nullptr)
                    AcceptNewConnection(readSocket, listenSocket);

                inet_stream* dataSocket = dynamic_cast<inet_stream*>(readSocket);
                if (dataSocket != nullptr)
                    HandleInput(readSocket, dataSocket, queryHandler);
            }

            for (auto& [readSocket, responseSocket] : ccSockets_)
            {
                inet_stream* dataSocket = dynamic_cast<inet_stream*>(readSocket);
                if (dataSocket != nullptr)
                    responseSocket->DoPeriodicSupport(queryHandler);
            }

            return false;
        }

    private:
        void AcceptNewConnection(socket* readSocket, inet_stream_server* listenSocket)
        {
            if (ccSockets_.find(readSocket) == end(ccSockets_))
            {
                cout << "QueryResponseListenSocket found readable socket is listen socket, creating new connection\n";
                auto dataSocket = make_unique<QueryResponseSocket>(listenSocket);
                ccSockets_[dataSocket->StreamSocket()] = std::move(dataSocket);
                MakeSelectSet();
            }
        }

        void HandleInput(socket* readSocket, inet_stream* dataSocket, unique_ptr<IQueryHandler> const & queryHandler)
        {
            cout << "QueryResponseListenSocket found readable data socket, handling request\n";
            auto iSocket = ccSockets_.find(readSocket);
            if (iSocket != end(ccSockets_))
            {
                if (!iSocket->second->HandleInput(queryHandler))
                {
                    cout << "QueryResponseListenSocket: readable data socket closed by client\n";
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
