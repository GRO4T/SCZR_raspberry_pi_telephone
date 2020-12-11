#include "audio.hpp"
#include "config.hpp"
#include <transmission.hpp>
#include <unistd.h>
#include <sys/wait.h>
#include <functional>
#include <wiringPi.h>
#include <thread>
#include <chrono>
#include <optional>
#include <vector>

using namespace std::chrono;

/*
 +-----+-----+---------+------+---+---Pi 4B--+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 |     |     |    3.3v |      |   |  1 || 2  |   |      | 5v      |     |     |
 |   2 |   8 |   SDA.1 |  OUT | 0 |  3 || 4  |   |      | 5v      |     |     |
 |   3 |   9 |   SCL.1 |   IN | 1 |  5 || 6  |   |      | 0v      |     |     |
 |   4 |   7 | GPIO. 7 |   IN | 1 |  7 || 8  | 1 | IN   | TxD     | 15  | 14  |
 |     |     |      0v |      |   |  9 || 10 | 1 | IN   | RxD     | 16  | 15  |
 |  17 |   0 | GPIO. 0 |  OUT | 0 | 11 || 12 | 0 | IN   | GPIO. 1 | 1   | 18  |
 |  27 |   2 | GPIO. 2 |   IN | 0 | 13 || 14 |   |      | 0v      |     |     |
 |  22 |   3 | GPIO. 3 |   IN | 0 | 15 || 16 | 0 | IN   | GPIO. 4 | 4   | 23  |
 |     |     |    3.3v |      |   | 17 || 18 | 0 | IN   | GPIO. 5 | 5   | 24  |
 |  10 |  12 |    MOSI |   IN | 0 | 19 || 20 |   |      | 0v      |     |     |
 |   9 |  13 |    MISO |   IN | 0 | 21 || 22 | 0 | IN   | GPIO. 6 | 6   | 25  |
 |  11 |  14 |    SCLK |   IN | 0 | 23 || 24 | 1 | IN   | CE0     | 10  | 8   |
 |     |     |      0v |      |   | 25 || 26 | 1 | IN   | CE1     | 11  | 7   |
 |   0 |  30 |   SDA.0 |   IN | 1 | 27 || 28 | 1 | IN   | SCL.0   | 31  | 1   |
 |   5 |  21 | GPIO.21 |   IN | 1 | 29 || 30 |   |      | 0v      |     |     |
 |   6 |  22 | GPIO.22 |   IN | 1 | 31 || 32 | 0 | IN   | GPIO.26 | 26  | 12  |
 |  13 |  23 | GPIO.23 |   IN | 0 | 33 || 34 |   |      | 0v      |     |     |
 |  19 |  24 | GPIO.24 |   IN | 0 | 35 || 36 | 0 | IN   | GPIO.27 | 27  | 16  |
 |  26 |  25 | GPIO.25 |   IN | 0 | 37 || 38 | 0 | IN   | GPIO.28 | 28  | 20  |
 |     |     |      0v |      |   | 39 || 40 | 0 | IN   | GPIO.29 | 29  | 21  |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+---Pi 4B--+---+------+---------+-----+-----+
 */

// wpi 8 : Enable / Disable sending
// wpi 9 : New packet send (on toggle)

#define ENABLE_PIN 8
#define NEW_PACKET_PIN 9
constexpr std::size_t VectorSize = 0x1'000'000;
constexpr std::size_t NumOfPacketsToCount = 0x1000;

std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>> send_at(VectorSize);
std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>> received_at(VectorSize);

void setup() {
    wiringPiSetup();
    
    pinMode(ENABLE_PIN, OUTPUT);
    pinMode(NEW_PACKET_PIN, INPUT);

    digitalWrite(ENABLE_PIN, LOW);
}

std::optional<uint32_t> try_recv_packet(auto & ptr) {
    {
      auto resource = ptr->lock();
      if (resource->empty())
        return {};

      return resource->pop_back()->reserved;
    }
    return {};
}

class D {
    uint32_t previous;
    uint32_t current;
    uint32_t pin;
public:
    D(uint32_t pin) : pin(pin) {
        previous = current = digitalRead(pin);
    }

    bool was_edge() {
        previous = current;
        current = digitalRead(pin);

        return current != previous;
    }
};

bool Halt = false;

void signal_handler(int sig) {
    if (sig == SIGINT) {
        Halt = true;
    }
}

void make_summary(std::size_t last) {
  std::chrono::time_point<std::chrono::high_resolution_clock> null_time_point;
  std::size_t lost_packets = 0;
  double delay_sum = 0.0;
  for (std::size_t i=0; i < last; ++i) {
    const auto send = send_at[i];
    const auto received = received_at[i];
    if (received == null_time_point) {
      printf("Packet %lu was lost\n", i);
      lost_packets++;
      continue;
    }
    const auto delay = std::chrono::duration_cast<std::chrono::microseconds>(received - send).count();
    delay_sum += delay;
    printf("Packet %lu was delayd by %ld microseconds\n", i, delay);
  }
  printf("%lu packets was lost and mean delay was %lf microseconds\n", lost_packets, delay_sum / (last - lost_packets));
}

void playAudio() {
  signal(SIGINT, &signal_handler);
  Audio<BUFFER_SIZE>::PacketDeque ptr(SHM_AUDIO_TEST_NAME);
  D d(NEW_PACKET_PIN);
  
  std::this_thread::sleep_for(1s);
  
  while (1) {
    {
      auto resource = ptr->lock();
      if (resource->empty())
        break;
      while (!resource->empty()) {
        resource->pop_back();
      }
    }
  }
  puts("Queue flushed");

  digitalWrite(ENABLE_PIN, HIGH);
  std::size_t i = 0;

  while (i < NumOfPacketsToCount && !Halt) {
    auto p = try_recv_packet(ptr);
    if (p && *p >= i) {
      throw std::runtime_error("WTF??? BACK TO THE FUTURE?");
    }
    if (p) {
      received_at[*p] = std::chrono::high_resolution_clock::now();
      printf("received packet with id = %d\n", *p);
    }
    if (d.was_edge()) {
      send_at[i++] = std::chrono::high_resolution_clock::now();
      printf("Packet %lu was send\n", i);
    }
  }
  make_summary(i);
}

void fetchDataFromMic() {
  Audio<BUFFER_SIZE>::PacketDeque ptr(SHM_AUDIO_TEST_NAME);
  Audio<BUFFER_SIZE>::AudioPacket audio_packet;
  transmission::MicPacket packet_from_mic;
  transmission::DataFromMicRetriever data_from_mic_retriever;

  while(1) {
    {
      auto resource = ptr->lock();
      if(resource->full())
        continue;
    }

    data_from_mic_retriever.fetchData(packet_from_mic);
    transmission::DataConverter::micPacketToAudioPacket(packet_from_mic, audio_packet);

    auto resource = ptr->lock();
    *resource->push_front() = audio_packet;
  }
}

int forkAndExecute(std::function<void()> func) {
  pid_t child_pid = fork();
  if (child_pid < 0) {
    throw std::runtime_error("Fork failed.");
  }
  else if (child_pid == 0) {
    func();
  }
  return child_pid;
}

int main() {
  setup();
  int pid = forkAndExecute(playAudio);
  if (pid > 0)
    pid = forkAndExecute(fetchDataFromMic);
  if (pid > 0) {
    wait(NULL);
  }
  return 0;
}
