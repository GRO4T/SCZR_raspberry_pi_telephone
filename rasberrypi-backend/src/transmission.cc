#include "transmission.hpp"
#include "protocol.pb.h"

#include <arpa/inet.h>
#include <cassert>
#include <thread>
#include <cstring>

namespace transmission {

void DataFromMicRetriever::fetchData(MicPacket &dest_packet) {
  if (!mic_synced) {
    handleDesynced(dest_packet);
  } else {
    handleSynced(dest_packet);
  }
}

void DataFromMicRetriever::handleSynced(MicPacket &dest_packet) {
  int n = mic.read(buffer_tmp, PACKET_SIZE);
  assert(n == PACKET_SIZE);
  memcpy((char *) &dest_packet.id, buffer_tmp, 4); // copy id
  if (!((dest_packet.id == PAC1 || dest_packet.id == PAC2) && (dest_packet.id != last_synced_id))) { // if id invalid
    handleDesynced(dest_packet);
    return;
  }
  last_synced_id = dest_packet.id;
  memcpy((char *) dest_packet.data, buffer_tmp + 4, PACKET_SIZE - 4); // copy rest of the packet
}

void DataFromMicRetriever::handleDesynced(MicPacket &dest_packet) {
  mic.read(buffer_tmp, PACKET_SIZE);
  // find id
  for (std::size_t i = 0; i < PACKET_SIZE; ++i) {
    uint32_t id = *reinterpret_cast<uint32_t *>(buffer_tmp + i);
    if (id == PAC1 || id == PAC2) {
      mic_synced = true;
      synced_at = i;
      last_synced_id = id;
      dest_packet.id = id;
      break;
    }
  }
  // handle rest of the packet
  std::size_t remaining_bytes = PACKET_SIZE - synced_at - 4;
  memcpy((char *) dest_packet.data, buffer_tmp + synced_at + 4, remaining_bytes);
  if (remaining_bytes < PACKET_SIZE - 4) { // more data needs to be fetched (we didn't sync at byte 0)
    const auto rest_of_the_bytes = mic.read(buffer_tmp, PACKET_SIZE - 4 - remaining_bytes);
    assert(rest_of_the_bytes == PACKET_SIZE - 4 - remaining_bytes);
    memcpy((char *) dest_packet.data + remaining_bytes, buffer_tmp, rest_of_the_bytes); // rest of the data
  }
}

const MicType &transmission::DataFromMicRetriever::getMic() const {
  return mic;
}

void DataConverter::micPacketToAudioPacket(MicPacket &src, Audio<BUFFER_SIZE>::AudioPacket &dest) {
  memcpy((char *) dest.data, (char *) src.data, DATA_SIZE);
  for (std::size_t i = 0; i < BUFFER_SIZE; ++i) {
    dest.data[i] -= constant_compound;
    dest.data[i] *= 3;
  }
}

DataTransmitter::DataTransmitter(const char* shm_name, ConnPtr conn) :
      state(ConnectionState::Start),
      conn(std::move(conn)),
      shared_memory_deque(shm_name) {

  /*
  IPv4 ip("127.0.0.1");
  int port = 8093;
  srv.bind(ip, port);
  srv.reuseaddr();
  srv.listen();
  client_conn = connect(ip, port);
  server_conn = srv.accept();
   */
  fd_selector.add(data_from_mic_retriever.getMic());
  fd_selector.addRead(*this->conn);
}

void DataTransmitter::transmit() {
  std::cout << "1";
  while (true) {
    fd_selector.wait(std::chrono::milliseconds(10));
    if (fd_selector.ready(data_from_mic_retriever.getMic())) {
      std::cout << "sending..." << std::endl;
      fetchFromMicAndSendOverNetwork();
    }
    //if (fd_selector.readyRead(socket)) {
    if (fd_selector.readyRead(*conn)) {
      std::cout << "receiving..." << std::endl;
      receiveFromNetworkAndPutInSharedMemory();
    }
  }
}

void DataTransmitter::fetchFromMicAndSendOverNetwork() {
  data_from_mic_retriever.fetchData(packet_out);
  assert(packet_out.id == PAC1 || packet_out.id == PAC2);

  Request request{};
  Data* data = new Data();
  data->set_data((char*)&packet_out, PACKET_SIZE);
  request.set_allocated_data(data);
  
  ProtocolData data_wrapped{};
  const auto result = request.SerializeToArray(data_wrapped.serialized_data, SERIALIZED_SIZE);
  assert(result);

  conn->write((char*)&data_wrapped, sizeof(data_wrapped));
}

void DataTransmitter::receiveFromNetworkAndPutInSharedMemory() {
  {
    auto resource = shared_memory_deque->lock();
    if(resource->full())
      return;
  }

  ProtocolData data_wrapped{};
  conn->read((char*)&data_wrapped, sizeof(data_wrapped));

  Response response{};
  const auto result = response.ParseFromArray(data_wrapped.serialized_data, SERIALIZED_SIZE);
  assert(result);

  if(response.Content_case() == Response::ContentCase::kData) {
    const auto& data = response.data();
    assert(data.data().size() == PACKET_SIZE);
    memcpy(&packet_in, data.data().data(), PACKET_SIZE);
    assert(packet_in.id == PAC1 || packet_in.id == PAC2);
    DataConverter::micPacketToAudioPacket(packet_in, audio_packet);

    auto resource = shared_memory_deque->lock();
    *resource->push_front() = audio_packet;
  }
}

}