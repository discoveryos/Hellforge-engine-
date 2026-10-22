#ifndef __HF_VULKAN_PROBE_H__
#define __HF_VULKAN_PROBE_H__

#include "hfStr.h"

// Vulkan availability probe for the renderer. Does NOT create a Vulkan device
// or take over rendering: it verifies the library loads, enumerates instance
// layer/extension count and physical devices, and reports the first device's
// name / API version / type into --no-op summary strings. Used to decide
// whether a future Vulkan backend (and the GL->Vulkan interop) is available
// on the machine before starting. Runs on demand (vulkanProbe console
// command) so a missing driver is harmless.

struct hfVulkanProbeResult {
	bool			loaded;		// dynamic library found + loadable
	bool			hasInstance;// vkCreateInstance + vkEnumeratePhysicalDevices ok
	unsigned int	apiVersion;	// VK_MAKE_API_VERSION-ish value (major<<22 | minor<<12 | patch)
	int				deviceCount;
	int				layerCount;	// instance layers enumerated
	hfStr			devices;	// "name | apiVersion | type" per line
};

class hfVulkanProbe {
public:
	// Only call from the main thread. Re-probes each call.
	hfVulkanProbeResult	Probe();
	hfStr				Summary();
};

extern hfVulkanProbe vulkanProbe;

// Registers the 'vulkanProbe' console command (called from R_InitOpenGL).
void HF_VulkanProbe_RegisterCommand();

#endif