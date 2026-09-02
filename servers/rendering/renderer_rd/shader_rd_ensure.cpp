/**************************************************************************/
/*  shader_rd_ensure.cpp                                                  */
/**************************************************************************/

#include "shader_rd.h"

// Separate translation unit so shader_rd.cpp stays untouched; see the fork's rule on keeping
// additions out of files that churn upstream.
void ShaderRD::ensure_version_compiled(RID p_version) {
	Version *version = version_owner.get_or_null(p_version);
	if (version == nullptr) {
		return;
	}
	_compile_ensure_finished(version);
}
