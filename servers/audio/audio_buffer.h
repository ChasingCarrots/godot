/**************************************************************************/
/*  audio_buffer.h                                                        */
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

#pragma once

#include "core/object/ref_counted.h"
#include "servers/audio/audio_frame.h"

class AudioBuffer : public RefCounted {
	GDCLASS(AudioBuffer, RefCounted);

	Vector<AudioFrame> data;

protected:
	static void _bind_methods();

public:
	static Ref<AudioBuffer> create(int p_frames);

	void resize(int p_frames);
	_FORCE_INLINE_ int get_frame_count() const { return data.size(); }
	_FORCE_INLINE_ int size() const { return data.size(); }
	_FORCE_INLINE_ bool is_empty() const { return data.is_empty(); }
	void clear();

	_FORCE_INLINE_ AudioFrame *ptrw() { return data.ptrw(); }
	_FORCE_INLINE_ const AudioFrame *ptr() const { return data.ptr(); }

	uint64_t get_data_address() const;

	Vector2 get_frame(int p_index) const;
	void set_frame(int p_index, const Vector2 &p_frame);

	AudioBuffer() = default;
	explicit AudioBuffer(int p_frames);
	~AudioBuffer() = default;
};
