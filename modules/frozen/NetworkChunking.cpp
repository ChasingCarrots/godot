#include "NetworkChunking.h"
#include "CommunicationLineSystem.h"
#include "CommunicationLine.h"
#include "core/error/error_macros.h"

//-------------------------------CHUNK SENDER------------------------------------

void ChunkSender::initialize(Ref<MultiplayerPeer> peer) {
    _send_buffer.instantiate();
    _multiplayer_peer = peer;
}

void ChunkSender::send_as_chunk(const int to, const PackedByteArray &packet) {
    ERR_FAIL_COND_MSG(_send_buffer.is_null(), "_send_buffer is null");
    ERR_FAIL_COND_MSG(_multiplayer_peer.is_null(), "_multiplayer_peer is null");
    ERR_FAIL_COND_MSG(packet.is_empty(), "Cannot chunk empty packet");
    ERR_FAIL_COND_MSG(packet.size() > MAX_CHUNK_SIZE, "Packet exceeds MAX_CHUNK_SIZE");

    const int num_slices = (packet.size() + SLICE_SIZE - 1) / SLICE_SIZE;
    ERR_FAIL_COND_MSG(num_slices > MAX_SLICES_PER_CHUNK, "Packet exceeds MAX_SLICES_PER_CHUNK");

    print_line("Start chunking packet of size: ", packet.size());

    _chunkId++;
    const uint8_t *src = packet.ptr();

    //Slice packet and send the slices
    for (int i = 0; i < num_slices; i++) {
        _send_buffer->clear();

        const int offset = i * SLICE_SIZE;
        const int bytes_to_copy = MIN(SLICE_SIZE, packet.size() - offset);

        _send_buffer->put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::SendDataChunk));
        _send_buffer->put_16(_chunkId);
        _send_buffer->put_32(num_slices);
        _send_buffer->put_32(i);
        _send_buffer->put_32(bytes_to_copy);
        _send_buffer->put_data(src + offset, bytes_to_copy);

        PackedByteArray slice = _send_buffer->get_data_array();

        _multiplayer_peer->set_target_peer(to);
        _multiplayer_peer->put_packet(slice.ptr(), slice.size());

        print_line("Send slice: ", i, " | with size: ", slice.size());
    }
}

//-------------------------------CHUNK RECEIVER------------------------------------

void ChunkReceiver::initialize(CommunicationLineSystem* cls) {
    _communication_line_system = cls;
}

void ChunkReceiver::receive_chunk_data(const int from, Ref<StreamPeerBuffer> buffer) {
    ERR_FAIL_COND_MSG(buffer.is_null(), "Buffer is null");

    const uint16_t packet_chunkId = buffer->get_u16();
    const int num_slices = buffer->get_32();
    const int current_slice_id = buffer->get_32();
    const int slice_size = buffer->get_32();

    ERR_FAIL_COND_MSG(num_slices <= 0 || num_slices > MAX_SLICES_PER_CHUNK, "Invalid num_slices");
    ERR_FAIL_COND_MSG(current_slice_id < 0 || current_slice_id >= num_slices, "Invalid slice index");
    ERR_FAIL_COND_MSG(slice_size < 0 || slice_size > SLICE_SIZE, "Invalid slice size");
    ERR_FAIL_NULL_MSG(_communication_line_system, "Communication Line System was not correctly initialized in ChunkReceiver");

    const int offset = current_slice_id * SLICE_SIZE;
    ERR_FAIL_COND_MSG(offset + slice_size > MAX_CHUNK_SIZE, "Chunk write exceeds MaxChunkSize");

    //We create a key based on the peer the packet is from and the chunk id
    uint64_t key = ((uint64_t)from << 32) | packet_chunkId;

    //Check if we already have a chunk with this id, if not create one
    auto it = _chunks.find(key);
    if (it == _chunks.end()) {
        print_line("Start receiving a chunk");

        ChunkData chunk{};
        chunk.received.resize_initialized(num_slices);
        chunk.data.resize_initialized(num_slices * SLICE_SIZE);
        chunk.num_slices = num_slices;
        chunk.total_size = 0;
        chunk.last_slice_time = OS::get_singleton()->get_ticks_msec();
        it = _chunks.emplace(key, std::move(chunk)).first;
    }

    ChunkData &chunkData = it->second;

    if (chunkData.num_slices != num_slices) {
        _chunks.erase(key);
        ERR_FAIL_MSG("Chunk num_slices mismatch");
    }

    print_line(vformat("Received slice %d of %d", current_slice_id + 1, num_slices));

    if (chunkData.received[current_slice_id]) {
        //If we receive the same slice again the data should normally be the same but just to be safe we will not overwrite it. But we update the last slice time.
        chunkData.last_slice_time = OS::get_singleton()->get_ticks_msec();
        return;
    }

    buffer->get_data(chunkData.data.ptr() + offset, slice_size);
    chunkData.received[current_slice_id] = true;
    chunkData.last_slice_time = OS::get_singleton()->get_ticks_msec();

    //The last chunk slice could have a different size as the other slices, therefore we need to wait until we received it to define the total size.
    if (current_slice_id == num_slices - 1) {
        chunkData.total_size = offset + slice_size;
    }

    for (int i = 0; i < num_slices; i++) {
        if (!chunkData.received[i]) {
            return;
        }
    }

    if (chunkData.total_size <= 0) {
        _chunks.erase(key);
        ERR_FAIL_MSG("Completed chunk missing total_size");
    }

    print_line("All slices received, continuing packet processing");
    _communication_line_system->process_packet(
        from, chunkData.data.ptr(), chunkData.total_size
    );

    _chunks.erase(key);
}

void ChunkReceiver::process() {
    if (_chunks.empty()){
        return;
    }

    const uint64_t now = OS::get_singleton()->get_ticks_msec();

    for (auto it = _chunks.begin(); it != _chunks.end(); ) {
        if (now - it->second.last_slice_time > CHUNK_LIFE_TIME_MS) {
            it = _chunks.erase(it);
        } else {
            ++it;
        }
    }
}