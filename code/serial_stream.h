#pragma once
extern "C"{
#include <fcntl.h>
#include <linux/netlink.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
}
#include <cerrno>
#include <cstring>
#include <ctime>
#include <string>

namespace Wootion{

enum Status{
  CONNECTED = 1,
  DISCONNECTED,
  ERROR
};

speed_t get_serial_baudrate(uint32_t rate);
 
class SerialStream{
 public:
  SerialStream(const char* device_name, speed_t baud_rate,
               uint32_t timeout_usec = 10000);
  ~SerialStream();

  bool Connect();
  bool Disconnect();

  size_t read(uint8_t* buffer, size_t max_length);
  size_t write(const uint8_t* data, size_t length);

  Status GetStatus();
 private:
  SerialStream() {}
  void open();
  void close();
  bool configure_port(int fd);
  bool wait_readable(uint32_t timeout_us);
  bool wait_writable(uint32_t timeout_us);
  void check_remove();

  Status status_;

  std::string device_name_;
  speed_t baud_rate_;
  uint32_t bytesize_;
  uint32_t parity_;
  uint32_t stopbits_;
  uint32_t flowcontrol_;
  uint32_t byte_time_us_;

  uint32_t timeout_usec_;
  int fd_;
  int errno_;
  bool is_open_;
};

} 
