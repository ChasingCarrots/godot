//===========================================================================//
// GodotSteam MultiplayerPeer - godotsteam_id.cpp
//===========================================================================//
//
// Copyright (c) 2017-Current | Ryan Leverenz and Contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//===========================================================================//

// Include GodotSteam header
#include "godotsteam_id.h"


void SteamID::_bind_methods() {
	ClassDB::bind_method(D_METHOD("from_int", "id"), &SteamID::from_int);

	ClassDB::bind_method(D_METHOD("get_account_id"), &SteamID::get_account_id);
	ClassDB::bind_method(D_METHOD("get_un_account_instance"), &SteamID::get_un_account_instance);
	ClassDB::bind_method(D_METHOD("get_account_type"), &SteamID::get_account_type);
	ClassDB::bind_method(D_METHOD("get_universe"), &SteamID::get_universe);

	ClassDB::bind_method(D_METHOD("to_int"), &SteamID::to_int);
	
	BIND_ENUM_CONSTANT(ACCOUNT_TYPE_INVALID);
	BIND_ENUM_CONSTANT(ACCOUNT_TYPE_INDIVIDUAL);
	BIND_ENUM_CONSTANT(ACCOUNT_TYPE_MULTISEAT);
	BIND_ENUM_CONSTANT(ACCOUNT_TYPE_GAME_SERVER);
	BIND_ENUM_CONSTANT(ACCOUNT_TYPE_ANON_GAME_SERVER);
	BIND_ENUM_CONSTANT(ACCOUNT_TYPE_PENDING);
	BIND_ENUM_CONSTANT(ACCOUNT_TYPE_CONTENT_SERVER);
	BIND_ENUM_CONSTANT(ACCOUNT_TYPE_CLAN);
	BIND_ENUM_CONSTANT(ACCOUNT_TYPE_CHAT);
	BIND_ENUM_CONSTANT(ACCOUNT_TYPE_CONSOLE_USER);
	BIND_ENUM_CONSTANT(ACCOUNT_TYPE_ANON_USER);

	BIND_ENUM_CONSTANT(UNIVERSE_INVALID);
	BIND_ENUM_CONSTANT(UNIVERSE_PUBLIC);
	BIND_ENUM_CONSTANT(UNIVERSE_BETA);
	BIND_ENUM_CONSTANT(UNIVERSE_INTERNAL);
	BIND_ENUM_CONSTANT(UNIVERSE_DEV);
}