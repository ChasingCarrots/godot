/**************************************************************************/
/*  audio_buffer.cpp                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "audio_buffer.h"

#include "core/object/class_db.h"

Ref<AudioBuffer> AudioBuffer::create(int p_frames) {
	ERR_FAIL_COND_V(p_frames < 0, nullptr);
	Ref<AudioBuffer> buffer;
	buffer.instantiate();
	buffer->resize(p_frames);
	return buffer;
}

void AudioBuffer::resize(int p_frames) {
	ERR_FAIL_COND(p_frames < 0);
	data.resize(p_frames);
}

void AudioBuffer::clear() {
	data.clear();
}

uint64_t AudioBuffer::get_data_address() const {
	return (uint64_t)(uintptr_t)data.ptr();
}

Vector2 AudioBuffer::get_frame(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, data.size(), Vector2());
	return Vector2(data[p_index].left, data[p_index].right);
}

void AudioBuffer::set_frame(int p_index, const Vector2 &p_frame) {
	ERR_FAIL_INDEX(p_index, data.size());
	data.ptrw()[p_index] = AudioFrame(p_frame.x, p_frame.y);
}

AudioBuffer::AudioBuffer(int p_frames) {
	if (p_frames > 0) {
		data.resize(p_frames);
	}
}

void AudioBuffer::_bind_methods() {
	ClassDB::bind_static_method("AudioBuffer", D_METHOD("create", "frames"), &AudioBuffer::create);

	ClassDB::bind_method(D_METHOD("resize", "frames"), &AudioBuffer::resize);
	ClassDB::bind_method(D_METHOD("get_frame_count"), &AudioBuffer::get_frame_count);
	ClassDB::bind_method(D_METHOD("size"), &AudioBuffer::size);
	ClassDB::bind_method(D_METHOD("is_empty"), &AudioBuffer::is_empty);
	ClassDB::bind_method(D_METHOD("clear"), &AudioBuffer::clear);

	ClassDB::bind_method(D_METHOD("get_data_address"), &AudioBuffer::get_data_address);
	ClassDB::bind_method(D_METHOD("get_frame", "index"), &AudioBuffer::get_frame);
	ClassDB::bind_method(D_METHOD("set_frame", "index", "frame"), &AudioBuffer::set_frame);
}
