#include "network_server.h"

namespace ArcticOwl {
namespace Modules {
namespace Network {

/**
 * @brief 构造函数
 * @details 初始化网络服务器，设置端口和接受器
 *
 * @param port 服务器监听端口
 */
NetworkServer::NetworkServer(int port)
    : m_acceptor(m_ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      m_running(false),
      m_port(port)
{
    // 设置接受器选项以允许地址重用
    m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
}

/**
 * @brief 析构函数
 * @details 停止网络服务并清理资源
 */
NetworkServer::~NetworkServer()
{
    stopNetworkSystem();
}

/**
 * @brief 启动网络服务
 * @details 启动服务器线程以接受客户端连接
 */
void NetworkServer::startNetworkSystem()
{
    // 如果已经在运行则返回
    if (m_running) {
        return;
    }

    m_running = true;
    // 启动服务器线程
    m_serverThread = std::thread([this]() {
        acceptConnections();
        m_ioContext.run();
    });
}

/**
 * @brief 停止网络服务
 * @details 停止服务器线程并关闭所有客户端连接
 */
void NetworkServer::stopNetworkSystem()
{
    // 如果服务器未运行则返回
    if (!m_running) {
        return;
    }

    m_running = false;
    m_acceptor.close();
    m_ioContext.stop();

    std::lock_guard<std::mutex> lock(m_clientsMutex); // 保护客户端列表
    // 关闭所有客户端连接
    for (auto& client : m_clients) {
        if (client->is_open()) {
            boost::system::error_code ec;
            client->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            client->close(ec);
        }
    }
    m_clients.clear();
    
    // 等待服务器线程结束
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
}

/**
 * @brief 接受客户端连接
 * @details 异步接受新的客户端连接并处理
 */
void NetworkServer::acceptConnections()
{
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(m_ioContext); // 创建新的套接字
    // 异步接受连接
    m_acceptor.async_accept(*socket, [this, socket](boost::system::error_code ec) {
        if (!ec) {
            std::cout << "新客户端连接: " << socket->remote_endpoint() << std::endl;

            std::lock_guard<std::mutex> lock(m_clientsMutex); // 保护客户端列表
            m_clients.push_back(socket); // 添加新客户端
            
            // 处理客户端
            handleClient(socket);
        }

        // 继续接受新的连接
        if (m_running) {
            acceptConnections();
        }
    });
}

/**
 * @brief 处理客户端通信
 * @details 异步读取客户端数据并处理断开连接
 *
 * @param socket 客户端套接字
 */
void NetworkServer::handleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    auto buffer = std::make_shared<std::vector<char>>(1024); // 创建缓冲区

    // 异步读取数据
    socket->async_read_some(boost::asio::buffer(*buffer)),
        [this, socket, buffer](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                // 处理接收到的数据（这里简化处理）
                std::string message(buffer->begin(), buffer->begin() + length);
                std::cout << "收到客户端消息: " << message << std::endl;

                // 继续读取
                handleClient(socket);
            } else {
                // 客户端断开连接
                std::cout << "客户端断开连接" << std::endl;

                // 从客户端列表中移除
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
    };
}

/**
 * @brief 广播视频帧
 * @details 将编码后的JPEG图像发送给所有连接的客户端
 *
 * @param frame 要广播的OpenCV图像帧
 */
void NetworkServer::broadcastFrame(const cv::Mat& frame)
{
    // 如果服务器未运行则返回
    if (!m_running) {
        return;
    }

    // 将图像编码为JPEG格式
    std::vector<uchar> buffer;
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 60};
    cv::imencode(".jpg", frame, buffer, params);

    // 发送给所有连接的客户端
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& client : m_clients) {
        try {
            // 发送数据大小
            uint32_t size = buffer.size();
            boost::asio::write(*client, boost::asio::buffer(&size, sizeof(size)));

            // 发送图像数据
            boost::asio::write(*client, boost::asio::buffer(buffer));
        } catch (const std::exception& e) {
            std::cerr << "发送数据到客户端时出错: " << e.what() << std::endl;
        }
    }
}

/**
 * @brief 发送警报消息
 * @details 将警报消息发送给所有连接的客户端
 *
 * @param alertMessage 要发送的警报消息
 */
void NetworkServer::sendAlert(const std::string& alertMessage)
{
    // 如果服务器未运行则返回
    if (!m_running) {
        return;
    }

    // 发送警报消息给所有连接的客户端
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& client : m_clients) {
        try {
            uint32_t size = alertMessage.size();
            boost::asio::write(*client, boost::asio::buffer(&size, sizeof(size)));
            boost::asio::write(*client, boost::asio::buffer(alertMessage));
        } catch (const std::exception& e) {
            std::cerr << "发送警报到客户端时出错: " << e.what() << std::endl;
        }
    }
}
    
} // namespace Network
} // namespace Modules
} // namespace ArcticOwl