def can_build(env, platform):
	return platform=="linuxbsd" or platform=="windows" or platform=="macos" or platform=="server" or platform=="android"

def configure(env):
	pass

def get_doc_classes():
	return [
		"Steam",
		"SteamMultiplayerPeer",
		"SteamPacketPeer"
	]

def get_doc_path():
	return "doc_classes"
