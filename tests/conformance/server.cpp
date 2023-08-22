#include <array>

#include <boost/ut.hpp>
#include <modbus/tcp.hpp>
#include <modbus/impl/deserialize_base.hpp>
#include <modbus/impl/deserialize_request.hpp>

/**
 * Modbus conformance test cases for server.
 */

#include <boost/asio.hpp>
#include <thread>
#include <condition_variable>
#include <mutex>

using boost::ut::operator""_test;
using boost::ut::expect;
using boost::ut::bdd::given;
using boost::ut::bdd::when;
using boost::ut::bdd::then;

using std::condition_variable;
using std::mutex;
using std::thread;
using std::unique_lock;
namespace asio = boost::asio;
using asio::awaitable;
using std::make_shared;
using std::shared_ptr;
using namespace std::chrono_literals;

struct state {
    state() : ul{mt} {}
    bool done;
    condition_variable cv;
    mutex mt;
    unique_lock<mutex> ul;
    size_t current_transaction = 0;
};

awaitable<void> t(shared_ptr<state> s, size_t transaction_id){
    // Send request out on the wire
    // ....

    // Wait for my transaction to come back
    s->cv.wait(s->ul, [s, transaction_id](){return s->current_transaction == transaction_id;});
    std::cout << "Got my transaction!" << std::endl;
}

awaitable<void> reader(shared_ptr<state> s){
    asio::steady_timer t(co_await asio::this_coro::executor);
    for(size_t i = 0; i < 100; i++){
        std::cout << "Transmitting transaction " << i << std::endl;
        s->current_transaction = i;
        s->cv.notify_all();
        t.expires_after(1s);
        co_await t.async_wait(asio::use_awaitable);
    }
}
int main(){
    auto s = make_shared<state>();
    asio::io_context ctx;
    co_spawn(ctx, t(s, 15), asio::detached);
    co_spawn(ctx, reader(s), asio::detached);
    ctx.run();
}
