#ifndef ARCTICOWL_MODULES_NETWORK_NETWORK_SERVER_H
#define ARCTICOWL_MODULES_NETWORK_NETWORK_SERVER_H

#include <boost/asio.hpp>
#include <thread>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <iostream>
#include <opencv2/opencv.hpp>

namespace ArcticOwl {
namespace Modules {
namespace Network {

/**
 * @brief The NetworkServer class
 * @details 使用Boost.Asio实现一个简单的TCP服务器，能够广播视频帧和发送警报消息
 */
class NetworkServer {
public:
    // 构造函数
    NetworkServer(int port);
    // 析构函数
    ~NetworkServer();

    // 启动网络服务器
    void startNetworkSystem();
    // 停止网络服务器
    void stopNetworkSystem();

    // 广播视频帧
    void broadcastFrame(const cv::Mat& frame);
    // 发送警报消息
    void sendAlert(const std::string& alertMessage);

private:
    // 接受连接
    void acceptConnections();
    // 处理客户端连接
    void handleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    boost::asio::io_context m_ioContext;                                  ///< IO上下文
    boost::asio::ip::tcp::acceptor m_acceptor;                            ///< TCP接收器
    std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>> m_clients; ///< 连接的客户端列表
    std::mutex m_clientsMutex;                                            ///< 保护客户端列表的互斥锁
    std::thread m_serverThread;                                           ///< 服务器线程
    std::atomic<bool> m_running;                                          ///< 服务器运行状态
    short m_port;
};

} // namespace Network
} // namespace Modules
} // namespace ArcticOwl

#endif // ARCTICOWL_MODULES_NETWORK_NETWORK_SERVER_H