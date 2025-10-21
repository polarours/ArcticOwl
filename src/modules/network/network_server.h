#ifndef ARCTICOWL_MODULES_NETWORK_NETWORK_SERVER_H
#define ARCTICOWL_MODULES_NETWORK_NETWORK_SERVER_H

#include <boost/asio.hpp>
#include <thread>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

namespace ArcticOwl {
namespace Modules {
namespace Network {

class NetworkServer {
public:
    NetworkServer(int port);
    ~NetworkServer();

    void startNetworkSystem();
    void stopNetworkSystem();

    void broadcastFrame(const cv::Mat& frame);
    void sendAlert(const std::string& alertMessage);

private:
    void acceptConnections();
    void handleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    boost::asio::io_context m_ioContext;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>> m_clients;
    std::mutex m_clientsMutex;
    std::thread m_serverThread;
    std::atomic<bool> m_running;
    short m_port;
};

} // namespace Network
} // namespace Modules
} // namespace ArcticOwl

#endif // ARCTICOWL_MODULES_NETWORK_NETWORK_SERVER_H