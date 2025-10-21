#include <algorithm>

#include "network_server.h"

namespace ArcticOwl {
namespace Modules {
namespace Network {

NetworkServer::NetworkServer(int port)
    : m_acceptor(m_ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      m_running(false),
      m_port(port)
{
    m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
}

NetworkServer::~NetworkServer()
{
    stopNetworkSystem();
}

void NetworkServer::startNetworkSystem()
{
    if (m_running) {
        return;
    }

    m_running = true;
    m_serverThread = std::thread([this]() {
        acceptConnections();
        m_ioContext.run();
    });
}

void NetworkServer::stopNetworkSystem()
{
    if (!m_running) {
        return;
    }

    m_running = false;
    m_acceptor.close();
    m_ioContext.stop();

    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& client : m_clients) {
        if (client->is_open()) {
            boost::system::error_code ec;
            client->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            client->close(ec);
        }
    }
    m_clients.clear();
    
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
}

void NetworkServer::acceptConnections()
{
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(m_ioContext);
    m_acceptor.async_accept(*socket, [this, socket](boost::system::error_code ec) {
        if (!ec) {
            std::cout << "Client connected: " << socket->remote_endpoint() << std::endl;

            std::lock_guard<std::mutex> lock(m_clientsMutex);
            m_clients.push_back(socket);
            
            handleClient(socket);
        }

        if (m_running) {
            acceptConnections();
        }
    });
}

void NetworkServer::handleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    auto buffer = std::make_shared<std::vector<char>>(1024);

    socket->async_read_some(boost::asio::buffer(*buffer),
        [this, socket, buffer](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::string message(buffer->begin(), buffer->begin() + length);
                std::cout << "Received message: " << message << std::endl;

                handleClient(socket);
            } else {
                std::cout << "Client disconnected" << std::endl;

                {
                    std::lock_guard<std::mutex> lock(m_clientsMutex);
                    auto it = std::find_if(m_clients.begin(), m_clients.end(),
                        [socket](const std::shared_ptr<boost::asio::ip::tcp::socket>& s) {
                            return s == socket;
                        });

                    if (it != m_clients.end()) {
                        m_clients.erase(it);
                    }
                }
            }
    });
}

void NetworkServer::broadcastFrame(const cv::Mat& frame)
{
    if (!m_running) {
        return;
    }

    std::vector<uchar> buffer;
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 60};
    cv::imencode(".jpg", frame, buffer, params);

    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& client : m_clients) {
        try {
            uint32_t size = buffer.size();
            boost::asio::write(*client, boost::asio::buffer(&size, sizeof(size)));

            boost::asio::write(*client, boost::asio::buffer(buffer));
        } catch (const std::exception& e) {
            std::cerr << "Failed to send frame: " << e.what() << std::endl;
        }
    }
}

void NetworkServer::sendAlert(const std::string& alertMessage)
{
    if (!m_running) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& client : m_clients) {
        try {
            uint32_t size = alertMessage.size();
            boost::asio::write(*client, boost::asio::buffer(&size, sizeof(size)));
            boost::asio::write(*client, boost::asio::buffer(alertMessage));
        } catch (const std::exception& e) {
            std::cerr << "Failed to send alert: " << e.what() << std::endl;
        }
    }
}
    
} // namespace Network
} // namespace Modules
} // namespace ArcticOwl