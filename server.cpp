#include <iostream>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>

using namespace boost::asio;
using namespace boost::posix_time;

#include "classes/node.cpp"

void new_nodes_thread() {
  ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 8001));
  while(true) {
    node_ptr new_(new node);
    acceptor.accept(new_->sock());
    boost::recursive_mutex::scoped_lock lk(cs);
    nodes.push_back(new_);
  }
}

void serve_nodes_thread() {
  while(true) {
    boost::this_thread::sleep(millisec(1));
    boost::recursive_mutex::scoped_lock lk(cs);
    for(array::iterator a = nodes.begin(), z = nodes.end(); a != z; ++a) {
      (*a)->serve_node();
    }
  }
}

void monitor_nodes_thread() {
  while(true) {
    boost::this_thread::sleep(millisec(100));
    boost::recursive_mutex::scoped_lock lk(cs);
    for(array::iterator a = nodes.begin(), z = nodes.end(); a != z; ++a) {
      (*a)->monitor_node();
    }
    nodes.erase(std::remove_if(nodes.begin(), nodes.end(), boost::bind(&node::timed_out,_1)), nodes.end());
  }
}

int main(int argc, char* argv[]) {
  boost::thread_group threads;
  threads.create_thread(new_nodes_thread);
  threads.create_thread(serve_nodes_thread);
  threads.create_thread(monitor_nodes_thread);
  threads.join_all();
}
