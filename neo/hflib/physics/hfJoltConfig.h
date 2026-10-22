#ifndef __HF_JOLT_CONFIG_H__
#define __HF_JOLT_CONFIG_H__

/*
================================

Jolt compile time configuration

Jolt hashes its compile time configuration into JPH_VERSION_ID and aborts in
RegisterTypes() if a library and its client disagree. The vendored copy under
external/jolt is always compiled with the set below, so the adapter pins the
same set before any Jolt header is pulled in. Building the engine with or
without -DHF_PHYSICS_HAS_JOLT therefore cannot silently disagree with the
vendored library.

================================
*/

#pragma once

#ifndef JPH_CROSS_PLATFORM_DETERMINISTIC
	#define JPH_CROSS_PLATFORM_DETERMINISTIC
#endif
#ifndef JPH_OBJECT_LAYER_BITS
	#define JPH_OBJECT_LAYER_BITS 16
#endif
#ifndef JPH_ENABLE_ASSERTS
	#define JPH_ENABLE_ASSERTS 0
#endif
#ifndef JPH_PROFILE_ENABLED
	#define JPH_PROFILE_ENABLED 0
#endif
#ifndef JPH_DEBUG_RENDERER
	#define JPH_DEBUG_RENDERER 0
#endif

#endif
