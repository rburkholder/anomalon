#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>

#include <sol/sol.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;

using tcp = asio::ip::tcp;

namespace Anomalon {

    // Master Lua State per OS thread to anchor global environments
    inline thread_local std::unique_ptr<sol::state> master_lua = nullptr;

    sol::state& get_master_lua() {
      if (!master_lua) {
        master_lua = std::make_unique<sol::state>();
        master_lua->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table);

        // Register C++ hooks here (e.g., database lookup bindings)

        master_lua->script_file("scripts/server.lua");
      }
      return *master_lua;
    }

    // Handle WebSocket routing using a dedicated Lua coroutine stack
    asio::awaitable<void> run_websocket_session(beast::ssl_stream<beast::tcp_stream> stream, sol::thread lua_thread) {
        try {
            websocket::stream<beast::ssl_stream<beast::tcp_stream>&> ws{stream};
            ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
            co_await ws.async_accept(asio::use_awaitable);

            // Create a view into our unique virtual Lua coroutine stack
            sol::state_view local_lua(lua_thread.state());
            beast::flat_buffer buffer;

            while(true) {
                // Thread may yield here and resume on a completely different pool worker
                co_await ws.async_read(buffer, asio::use_awaitable);

                std::string input = beast::buffers_to_string(buffer.data());
                sol::protected_function handler = local_lua["on_websocket_message"];
                std::string response = "Error processing request";

                if (handler.valid()) {
                    sol::protected_function_result result = handler(input);
                    if (result.valid()) {
                        response = result.get<std::string>();
                    }
                }

                ws.text(ws.got_text());
                co_await ws.async_write(asio::buffer(response), asio::use_awaitable);
                buffer.consume(buffer.size());
            }
        } catch (const boost::system::system_error& se) {
            if (se.code() != websocket::error::closed)
                std::cerr << "[WS Error] " << se.what() << "\n";
        }
    }

    // Handle standard HTTPS and WebSocket upgrades stateless-ly
    asio::awaitable<void> handle_session(tcp::socket socket, asio::ssl::context& ssl_ctx) {
        try {
            // 1. Get the worker thread's master Lua instance to branch from
            auto& master = get_master_lua();

            // 2. Spawn a lightweight Lua thread (virtual stack context) for this connection.
            // This is entirely internal to Lua and doesn't allocate OS threads.
            sol::thread lua_thread = sol::thread::create(master);
            sol::state_view local_lua(lua_thread.state());

            beast::ssl_stream<beast::tcp_stream> stream{std::move(socket), ssl_ctx};
            beast::flat_buffer buffer;

            // Perform SSL Handshake (coroutine can safely yield and switch OS threads)
            co_await stream.async_handshake(asio::ssl::stream_base::server, asio::use_awaitable);

            while(true) {
                http::request<http::string_body> req;
                co_await http::async_read(stream, buffer, req, asio::use_awaitable);

                // Hand over execution to the WebSocket loop if an upgrade is requested
                if(websocket::is_upgrade(req)) {
                    co_await run_websocket_session(std::move(stream), std::move(lua_thread));
                    co_return;
                }

                std::string path = std::string(req.target());
                sol::protected_function handler = local_lua["route_http"];
                std::string body = "404 Not Found";
                http::status status = http::status::not_found;

                if (handler.valid()) {
                    sol::protected_function_result result = handler(path);
                    if (result.valid()) {
                        body = result.get<std::string>();
                        status = http::status::ok;
                    }
                }

                http::response<http::string_body> res{status, req.version()};
                res.set(http::field::server, "Anomalon Core/1.0");
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = body;
                res.prepare_payload();

                co_await http::async_write(stream, res, asio::use_awaitable);

                if(!req.keep_alive()) break;
            }

            beast::error_code ec;
            co_await stream.async_shutdown(asio::use_awaitable);
        } catch (const std::exception& e) {
            std::cerr << "[Session Exception] " << e.what() << "\n";
        }
    }

    // Network listener accepting connections and distributing directly to the global pool
    asio::awaitable<void> listener(unsigned short port, asio::ssl::context& ctx) {
        auto executor = co_await asio::this_coro::executor;
        tcp::acceptor acceptor{executor, {tcp::v4(), port}};

        std::cout << "[Anomalon] Statelessly balancing secure traffic on port " << port << "...\n";

        while(true) {
            tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);

            // No strands used here. Spawning directly onto the global context executor
            // allows connections to aggressively steal and utilize any free worker thread.
            asio::co_spawn(executor, handle_session(std::move(socket), ctx), asio::detached);
        }
    }
}

int main() {
  try {
    asio::io_context ioc;

    asio::ssl::context ctx{asio::ssl::context::tlsv13};
    //ctx.use_certificate_chain_file("server.crt");
    //ctx.use_private_key_file("server.key", asio::ssl::context::pem);

    // Start the master listener
    asio::co_spawn(ioc, Anomalon::listener(443, ctx), asio::detached);

    // Spin up the hardware-optimized thread pool
    //unsigned int thread_count = std::thread::hardware_concurrency();
    unsigned int thread_count = 2;
    std::vector<std::thread> thread_pool;
    thread_pool.reserve(thread_count - 1);

    std::cout << "[Anomalon] Initializing stateless engine with pool size: " << thread_count << "\n";

    for( unsigned int i = 0; i < thread_count - 1; ++i) {
        thread_pool.emplace_back([&ioc]() {
            ioc.run();
        });
    }
    ioc.run();

    for(auto& t : thread_pool) {
        if(t.joinable()) t.join();
    }

  } catch (const std::exception& e) {
    std::cerr << "[Critical Exception] " << e.what() << "\n";
    return 1;
  }
  return 0;
}
