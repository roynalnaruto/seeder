#include <iostream>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>

using namespace boost::asio;
using namespace boost::posix_time;

struct node;
io_service service;
typedef boost::shared_ptr<node> node_ptr;
typedef std::vector<node_ptr> array;
array nodes;

time_duration ONE_HOUR = hours(1);
time_duration ONE_DAY = hours(24);

boost::recursive_mutex cs;

struct node : boost::enable_shared_from_this<node> {
  node() : sock_(service), already_read_(0) {
  }

  void serve_node() {
    try {
      read_query();
      process_query();
    } catch(boost::system::system_error & err) {
      std::cout<<err.what();
      stop();
    }
  }

  void monitor_node() {
    if (timed_out()) {
      stop();
    }
  }

  ip::tcp::socket & sock() {
    return sock_;
  }

  std::string get_ip() {
    return (sock_.remote_endpoint().address().to_string() + ":" + std::to_string(sock_.remote_endpoint().port()));
  }

  bool timed_out() {
    ptime now = microsec_clock::local_time();
    long long ms = (now - last_ping_).total_milliseconds();
    return (ms > 10000);
  }

  long long get_alive_since() {
    time_duration life = last_ping_ - join_time_;
    return life.total_milliseconds();
  }

  void stop() {
    boost::system::error_code err;
    sock_.close(err);
  }

private:
  void read_query() {
    if (sock_.available()) {
      already_read_ += sock_.read_some(buffer(buff_ + already_read_, max_msg - already_read_));
    }
  }

  void process_query() {
    bool found_endofline = std::find(buff_, buff_ + already_read_, '\n') < buff_ + already_read_;
    if (!found_endofline) {
      return;
    }
    last_ping_ = microsec_clock::local_time();
    size_t pos = std::find(buff_, buff_ + already_read_, '\n') - buff_;
    std::string msg(buff_, pos);
    std::copy(buff_ + already_read_, buff_ + max_msg, buff_);
    already_read_ -= pos + 1;

    if (msg.find("hello") == 0) on_join();
    else if (msg.find("list_all") == 0) on_list_all_nodes();
    else if (msg.find("ping") == 0) on_ping();
    else if (msg.find("list_1_hour") == 0) on_list_nodes_alive_for(ONE_HOUR.total_milliseconds());
    else if (msg.find("list_2_hour") == 0) on_list_nodes_alive_for(2*ONE_HOUR.total_milliseconds());
    else if (msg.find("list_1_day") == 0) on_list_nodes_alive_for(ONE_DAY.total_milliseconds());
    else std::cerr << "invalid msg " << msg << std::endl;
  }

  void on_ping() {
    last_ping_ = microsec_clock::local_time();
  }

  void on_join() {
    join_time_ = microsec_clock::local_time();
    last_ping_ = microsec_clock::local_time();
    std::cout<<"Node: "<<get_ip()<<" joined"<<std::endl;
    write("hello from seeder\n");
  }

  void on_list_all_nodes() {
    std::string write_msg("All nodes: ");
    {
      boost::recursive_mutex::scoped_lock lk(cs);
      for(array::const_iterator a = nodes.begin(), z = nodes.end(); a != z; ++a) {
        write_msg += (*a)->get_ip() + ", ";
      }
    }
    write_msg += "\n";
    write(write_msg);
  }

  void on_list_nodes_alive_for(long long query_time) {
    std::string write_msg("Nodes alive: ");
    {
      boost::recursive_mutex::scoped_lock lk(cs);
      for(array::const_iterator a = nodes.begin(), z = nodes.end(); a != z; ++a) {
        if ((*a)->get_alive_since() > query_time) {
          write_msg += (*a)->get_ip() + ", ";
        }
      }
    }
    write_msg += "\n";
    write(write_msg);
  }

  void write(const std::string & msg) {
    sock_.write_some(buffer(msg));
  }

private:
  ip::tcp::socket sock_;
  ptime join_time_;
  ptime last_ping_;
  enum { max_msg = 1024 };
  int already_read_;
  char buff_[max_msg];
};

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
