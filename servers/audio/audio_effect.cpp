/**************************************************************************/
/*  audio_effect.cpp                                                      */
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

#include "audio_effect.h"

PackedVector2Array AudioEffectInstance::_process_audio_bind(const PackedVector2Array &p_src_buffer, int p_frame_count) {
	int count = MIN(p_frame_count, p_src_buffer.size());
	// keep the src and dst buffers around as thread_local to minimize memory allocations between calls.
	thread_local LocalVector<AudioFrame> src;
	thread_local LocalVector<AudioFrame> dst;
	src.resize(count);
	dst.resize(count);

	AudioFrame *src_ptrw = src.ptr();
	const Vector2 *src_r = p_src_buffer.ptr();
	for (int i = 0; i < count; i++) {
		src_ptrw[i] = {src_r[i].x, src_r[i].y};
	}

	process(src.ptr(), dst.ptr(), count);

	PackedVector2Array res;
	res.resize(count);
	Vector2 *res_ptrw = res.ptrw();
	const AudioFrame *dst_r = dst.ptr();
	for (int i = 0; i < count; i++) {
		res_ptrw[i] = Vector2(dst_r[i].left, dst_r[i].right);
	}

	// when the buffers are abnormally large, deallocate them. We don't want to waste memory unnecessarily.
	if (src.size() > 2048) {
		src.reset();
	}
	if (dst.size() > 2048) {
		dst.reset();
	}

	return res;
}

void AudioEffectInstance::process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) {
	GDVIRTUAL_CALL(_process, p_src_frames, p_dst_frames, p_frame_count);
}
bool AudioEffectInstance::process_silence() const {
	bool ret = false;
	GDVIRTUAL_CALL(_process_silence, ret);
	return ret;
}

void AudioEffectInstance::_bind_methods() {
	GDVIRTUAL_BIND(_process, "src_buffer", "dst_buffer", "frame_count");
	GDVIRTUAL_BIND(_process_silence);

	ClassDB::bind_method(D_METHOD("process_audio", "src_buffer", "frame_count"),
		&AudioEffectInstance::_process_audio_bind);
	ClassDB::bind_method(D_METHOD("process_silence"), &AudioEffectInstance::process_silence);
}

////

Ref<AudioEffectInstance> AudioEffect::instantiate() {
	Ref<AudioEffectInstance> ret;
	GDVIRTUAL_CALL(_instantiate, ret);
	return ret;
}
void AudioEffect::_bind_methods() {
	GDVIRTUAL_BIND(_instantiate);

	ClassDB::bind_method(D_METHOD("instantiate"), &AudioEffect::instantiate);
}

AudioEffect::AudioEffect() {
}
