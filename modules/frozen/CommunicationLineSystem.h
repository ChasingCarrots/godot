#ifndef COMMUNICATIONLINESYSTEM_H
#define COMMUNICATIONLINESYSTEM_H

#include "CommunicationLine.h"

#include <modules/multiplayer/scene_multiplayer.h>
#include <scene/main/node.h>

class CommunicationLineSystem : public Node {
	GDCLASS(CommunicationLineSystem, Node)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();
	void _notification(int p_notification);

private:
	friend CommunicationLine;
	Ref<SceneMultiplayer> _scene_multiplayer;
	bool is_server() const { return _scene_multiplayer->is_server(); }
	uint32_t _next_line_id = 1;
	Vector<Ref<CommunicationLine>> _communication_lines;
	Ref<StreamPeerBuffer> _receive_buffer;
	Ref<StreamPeerBuffer> _send_buffer;

	void _ready();
	void on_new_peer_connected(int multiplayer_id);
	void on_packet_received(int from_multiplayer_id, const PackedByteArray& packet);
	void send_packet_to_peer(const PackedByteArray& bytes, int peer_id, MultiplayerPeer::TransferMode mode) const;
	void send_packet_to_server(const PackedByteArray& bytes, MultiplayerPeer::TransferMode mode) const;
public:
	Ref<CommunicationLine> grab_communication_line(const StringName &id);

	// access to all lines mostly for debug visualization purposes:
	int get_number_of_communication_lines() const { return _communication_lines.size(); }
	Ref<CommunicationLine> get_communication_line(int index) const {
		if (index < 0 || index >= _communication_lines.size()) { return {};}
		return _communication_lines[index];
	}
};

#endif //COMMUNICATIONLINESYSTEM_H
