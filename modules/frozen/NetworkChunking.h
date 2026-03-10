#ifndef NETWORKCHUNKING_H
#define NETWORKCHUNKING_H

#include "core/io/stream_peer.h"
#include "core/object/ref_counted.h"
#include "core/templates/local_vector.h"
#include "scene/main/multiplayer_peer.h"

#include <cstdint>
#include <unordered_map>

constexpr int SLICE_SIZE = 1024;
constexpr int MAX_SLICES_PER_CHUNK = 256;
constexpr int MAX_CHUNK_SIZE = SLICE_SIZE * MAX_SLICES_PER_CHUNK;
constexpr uint64_t CHUNK_LIFE_TIME_MS = 20000;

struct ChunkData {
    LocalVector<uint8_t> received;
    LocalVector<uint8_t> data;
    int total_size = 0;
    int num_slices = 0;
    uint64_t last_slice_time = 0;
};

class ChunkSender : public RefCounted
{
    uint16_t _chunkId = 0;
    Ref<StreamPeerBuffer> _send_buffer;
    Ref<MultiplayerPeer> _multiplayer_peer;

public:
    void initialize(Ref<MultiplayerPeer> peer);
    void send_as_chunk(const int to, const PackedByteArray &packet);
};

class CommunicationLineSystem;
class ChunkReceiver : public RefCounted
{
    std::unordered_map<uint64_t, ChunkData> _chunks;
    CommunicationLineSystem* _communication_line_system = nullptr;

public:
    void initialize(CommunicationLineSystem* cls);
    void receive_chunk_data(const int from, Ref<StreamPeerBuffer> buffer);

    void process();
};

#endif //NETWORKCHUNKING_H