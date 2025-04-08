//===========================================================================//
// GodotSteam - register_types.cpp
//===========================================================================//
//
// Copyright (c) 2015-Current | GP Garcia and Contributors (view contributors.md)
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

#include "register_types.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/object/class_db.h"
#include "godotsteam.h"


static Steam *SteamPtr = nullptr;


void initialize_godotsteam_module(ModuleInitializationLevel level){
	if(level == MODULE_INITIALIZATION_LEVEL_SERVERS){
		GDREGISTER_CLASS(Steam);
		SteamPtr = memnew(Steam);
		Engine::get_singleton()->add_singleton(Engine::Singleton("Steam", Steam::get_singleton()));
 
		// Setup Project Settings
		int app_id = GLOBAL_DEF_BASIC("steam/initialization/app_id", 0);
		bool auto_init = GLOBAL_DEF_BASIC("steam/initialization/initialize_on_startup", false);
		bool embed_callbacks = GLOBAL_DEF_BASIC("steam/initialization/embed_callbacks", false);

		if (auto_init){
			// Prevent intializing Steam from the editor itself
			if (Engine::get_singleton()->is_editor_hint()) {
				return;
			}
			Steam::get_singleton()->steamInitEx(app_id, embed_callbacks);
		}
	}
}


void uninitialize_godotsteam_module(ModuleInitializationLevel level){
	if(level == MODULE_INITIALIZATION_LEVEL_SERVERS){
		Engine::get_singleton()->remove_singleton("Steam");
		memdelete(SteamPtr);
	}
}
