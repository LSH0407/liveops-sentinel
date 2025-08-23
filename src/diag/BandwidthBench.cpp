#include "BandwidthBench.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <numeric>

BandwidthBench::BandwidthBench() {
    io_context_ = std::make_unique<asio::io_context>();
}

BandwidthBench::~BandwidthBench() {
    stop();
}

bool BandwidthBench::start(const BenchConfig& config, BenchCallback callback) {
    if (running_) {
        stop();
    }
    
    config_ = config;
    callback_ = std::move(callback);
    running_ = true;
    
    bench_thread_ = std::thread([this]() {
        if (config_.mode == BenchMode::SERVER) {
            runServer();
        } else {
            runClient();
        }
    });
    
    spdlog::info("Bandwidth benchmark started: {} mode, {} protocol", 
                 config_.mode == BenchMode::SERVER ? "SERVER" : "CLIENT",
                 config_.protocol == BenchProtocol::UDP ? "UDP" : "TCP");
    
    return true;
}

void BandwidthBench::stop() {
    if (running_) {
        running_ = false;
        if (bench_thread_.joinable()) {
            bench_thread_.join();
        }
        
        if (tcp_acceptor_) {
            tcp_acceptor_->close();
        }
        if (udp_socket_) {
            udp_socket_->close();
        }
        
        spdlog::info("Bandwidth benchmark stopped");
    }
}

bool BandwidthBench::isRunning() const {
    return running_;
}

BenchResult BandwidthBench::getServerStats() const {
    std::lock_guard<std::mutex> lock(server_stats_mutex_);
    return server_stats_;
}

void BandwidthBench::runServer() {
    if (config_.protocol == BenchProtocol::UDP) {
        runUdpServer();
    } else {
        runTcpServer();
    }
}

void BandwidthBench::runClient() {
    if (config_.protocol == BenchProtocol::UDP) {
        runUdpClient();
    } else {
        runTcpClient();
    }
}

void BandwidthBench::runUdpServer() {
    try {
        udp_socket_ = std::make_unique<asio::ip::udp::socket>(*io_context_);
        udp_socket_->open(asio::ip::udp::v4());
        udp_socket_->bind(asio::ip::udp::endpoint(asio::ip::address_v4::any(), config_.targetPort));
        
        std::vector<char> buffer(config_.packetSize);
        asio::ip::udp::endpoint sender_endpoint;
        
        auto start_time = std::chrono::steady_clock::now();
        int64_t total_bytes = 0;
        int packet_count = 0;
        
        while (running_) {
            asio::error_code ec;
            size_t bytes_received = udp_socket_->receive_from(asio::buffer(buffer), sender_endpoint, 0, ec);
            
            if (!ec && bytes_received > 0) {
                total_bytes += bytes_received;
                packet_count++;
                
                auto now = std::chrono::steady_clock::now();
                {
                    std::lock_guard<std::mutex> lock(server_stats_mutex_);
                    packet_timestamps_.push_back(now);
                    
                    // 최근 100개 패킷만 유지
                    if (packet_timestamps_.size() > 100) {
                        packet_timestamps_.erase(packet_timestamps_.begin());
                    }
                    
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
                    if (duration > 0) {
                        server_stats_.downlinkMbps = (total_bytes * 8.0) / (duration * 1000.0);
                        server_stats_.receivedPackets = packet_count;
                        server_stats_.timestamp = now;
                    }
                }
                
                // 에코 응답
                udp_socket_->send_to(asio::buffer(buffer.data(), bytes_received), sender_endpoint, 0, ec);
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("UDP server error: {}", e.what());
    }
}

void BandwidthBench::runTcpServer() {
    try {
        tcp_acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(*io_context_);
        tcp_acceptor_->open(asio::ip::tcp::v4());
        tcp_acceptor_->set_option(asio::ip::tcp::acceptor::reuse_address(true));
        tcp_acceptor_->bind(asio::ip::tcp::endpoint(asio::ip::address_v4::any(), config_.targetPort));
        tcp_acceptor_->listen();
        
        while (running_) {
            asio::ip::tcp::socket socket(*io_context_);
            tcp_acceptor_->accept(socket);
            
            std::vector<char> buffer(config_.packetSize);
            auto start_time = std::chrono::steady_clock::now();
            int64_t total_bytes = 0;
            int packet_count = 0;
            
            while (running_) {
                asio::error_code ec;
                size_t bytes_received = socket.read_some(asio::buffer(buffer), ec);
                
                if (ec || bytes_received == 0) break;
                
                total_bytes += bytes_received;
                packet_count++;
                
                // 에코 응답
                asio::write(socket, asio::buffer(buffer.data(), bytes_received), ec);
                if (ec) break;
                
                auto now = std::chrono::steady_clock::now();
                {
                    std::lock_guard<std::mutex> lock(server_stats_mutex_);
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
                    if (duration > 0) {
                        server_stats_.downlinkMbps = (total_bytes * 8.0) / (duration * 1000.0);
                        server_stats_.receivedPackets = packet_count;
                        server_stats_.timestamp = now;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("TCP server error: {}", e.what());
    }
}

void BandwidthBench::runUdpClient() {
    try {
        udp_socket_ = std::make_unique<asio::ip::udp::socket>(*io_context_);
        udp_socket_->open(asio::ip::udp::v4());
        
        asio::ip::udp::resolver resolver(*io_context_);
        auto endpoints = resolver.resolve(config_.targetHost, std::to_string(config_.targetPort));
        auto endpoint = *endpoints.begin();
        
        std::vector<char> buffer(config_.packetSize);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        // 패킷에 타임스탬프 포함
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::seconds(config_.durationSec);
        
        int packet_id = 0;
        int64_t total_sent = 0;
        int64_t total_received = 0;
        
        while (running_ && std::chrono::steady_clock::now() < end_time) {
            auto send_time = std::chrono::steady_clock::now();
            
            // 패킷 헤더에 ID와 타임스탬프 포함
            int64_t timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                send_time.time_since_epoch()).count();
            
            memcpy(buffer.data(), &packet_id, sizeof(packet_id));
            memcpy(buffer.data() + sizeof(packet_id), &timestamp_ns, sizeof(timestamp_ns));
            
            // 나머지는 랜덤 데이터
            for (size_t i = sizeof(packet_id) + sizeof(timestamp_ns); i < buffer.size(); ++i) {
                buffer[i] = static_cast<char>(dis(gen));
            }
            
            asio::error_code ec;
            udp_socket_->send_to(asio::buffer(buffer), endpoint, 0, ec);
            if (!ec) {
                total_sent++;
                sent_timestamps_.push_back(timestamp_ns);
            }
            
            // 응답 수신
            asio::ip::udp::endpoint sender_endpoint;
            size_t bytes_received = udp_socket_->receive_from(asio::buffer(buffer), sender_endpoint, 0, ec);
            
            if (!ec && bytes_received >= sizeof(packet_id) + sizeof(timestamp_ns)) {
                auto recv_time = std::chrono::steady_clock::now();
                int64_t recv_timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    recv_time.time_since_epoch()).count();
                
                int recv_packet_id;
                int64_t sent_timestamp_ns;
                memcpy(&recv_packet_id, buffer.data(), sizeof(recv_packet_id));
                memcpy(&sent_timestamp_ns, buffer.data() + sizeof(recv_packet_id), sizeof(sent_timestamp_ns));
                
                if (recv_packet_id == packet_id) {
                    total_received++;
                    double rtt_ms = (recv_timestamp_ns - sent_timestamp_ns) / 1000000.0;
                    rtt_samples_.push_back(rtt_ms);
                    received_timestamps_.push_back(recv_timestamp_ns);
                }
            }
            
            packet_id++;
            
            // 패킷 전송률 제어
            std::this_thread::sleep_for(std::chrono::microseconds(1000000 / config_.packetsPerSec));
        }
        
        // 결과 계산
        BenchResult result;
        result.totalPackets = total_sent;
        result.receivedPackets = total_received;
        result.lossPct = total_sent > 0 ? (1.0 - (double)total_received / total_sent) * 100.0 : 0.0;
        
        if (!rtt_samples_.empty()) {
            result.rttMsAvg = std::accumulate(rtt_samples_.begin(), rtt_samples_.end(), 0.0) / rtt_samples_.size();
            result.rttMsMin = *std::min_element(rtt_samples_.begin(), rtt_samples_.end());
            result.rttMsMax = *std::max_element(rtt_samples_.begin(), rtt_samples_.end());
            
            // 지터 계산
            double sum_squared_diff = 0.0;
            for (double rtt : rtt_samples_) {
                sum_squared_diff += (rtt - result.rttMsAvg) * (rtt - result.rttMsAvg);
            }
            result.jitterMs = std::sqrt(sum_squared_diff / rtt_samples_.size());
        }
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
        if (duration > 0) {
            result.uplinkMbps = (total_sent * config_.packetSize * 8.0) / (duration * 1000.0);
        }
        
        result.timestamp = std::chrono::steady_clock::now();
        
        if (callback_) {
            callback_(result);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("UDP client error: {}", e.what());
    }
}

void BandwidthBench::runTcpClient() {
    try {
        asio::ip::tcp::resolver resolver(*io_context_);
        auto endpoints = resolver.resolve(config_.targetHost, std::to_string(config_.targetPort));
        
        asio::ip::tcp::socket socket(*io_context_);
        asio::connect(socket, endpoints);
        
        std::vector<char> buffer(config_.packetSize);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::seconds(config_.durationSec);
        
        int64_t total_sent = 0;
        int64_t total_received = 0;
        
        while (running_ && std::chrono::steady_clock::now() < end_time) {
            auto send_time = std::chrono::steady_clock::now();
            
            // 랜덤 데이터 생성
            for (char& byte : buffer) {
                byte = static_cast<char>(dis(gen));
            }
            
            asio::error_code ec;
            asio::write(socket, asio::buffer(buffer), ec);
            if (!ec) {
                total_sent++;
            }
            
            // 응답 수신
            size_t bytes_received = socket.read_some(asio::buffer(buffer), ec);
            if (!ec && bytes_received > 0) {
                total_received++;
                
                auto recv_time = std::chrono::steady_clock::now();
                double rtt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    recv_time - send_time).count();
                rtt_samples_.push_back(rtt_ms);
            }
        }
        
        // 결과 계산
        BenchResult result;
        result.totalPackets = total_sent;
        result.receivedPackets = total_received;
        result.lossPct = total_sent > 0 ? (1.0 - (double)total_received / total_sent) * 100.0 : 0.0;
        
        if (!rtt_samples_.empty()) {
            result.rttMsAvg = std::accumulate(rtt_samples_.begin(), rtt_samples_.end(), 0.0) / rtt_samples_.size();
            result.rttMsMin = *std::min_element(rtt_samples_.begin(), rtt_samples_.end());
            result.rttMsMax = *std::max_element(rtt_samples_.begin(), rtt_samples_.end());
        }
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
        if (duration > 0) {
            result.uplinkMbps = (total_sent * config_.packetSize * 8.0) / (duration * 1000.0);
        }
        
        result.timestamp = std::chrono::steady_clock::now();
        
        if (callback_) {
            callback_(result);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("TCP client error: {}", e.what());
    }
}

void BandwidthBench::updateStats(const BenchResult& result) {
    if (callback_) {
        callback_(result);
    }
}
