#ifndef GODOT_SOURCE_REGISTER_TYPES_H
#define GODOT_SOURCE_REGISTER_TYPES_H

#include <modules/register_module_types.h>

void initialize_HOT_module(ModuleInitializationLevel p_level);
void uninitialize_HOT_module(ModuleInitializationLevel p_level);
/* yes, the word in the middle must be the same as the module folder name */

#endif //GODOT_SOURCE_REGISTER_TYPES_H
