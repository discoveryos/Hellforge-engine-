#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfVulkanProbe.h"

#include "hfCmdSystem.h"
#include "hfCVarSystem.h"
#include "hfLib.h"

#include <dlfcn.h>

// Minimal subset of the Vulkan API used by the probe. Casting dlsym results
// back into function pointers of this shape is the documented way to call GL
// and Vulkan through dynamically resolved entry points without linking.
#define VK_DEFINE_HANDLE( object ) typedef struct object##_T * object;
VK_DEFINE_HANDLE( VkInstance )
VK_DEFINE_HANDLE( VkPhysicalDevice )

typedef void * ( *PFN_vkGetInstanceProcAddr ) ( VkInstance instance, const char * pName );
typedef void ( *PFN_vkDestroyInstance ) ( VkInstance instance, const void * pAllocator );
typedef int ( *PFN_vkEnumerateInstanceLayerPropertiesProc ) ( unsigned int * pPropertyCount, void * pProperties );
typedef int ( *PFN_vkCreateInstance ) ( const void * pCreateInfo, const void * pAllocator, VkInstance * pInstance );
typedef int ( *PFN_vkEnumeratePhysicalDevices ) ( VkInstance instance, unsigned int * pPhysicalDeviceCount, VkPhysicalDevice * pPhysicalDevices );
typedef void ( *PFN_vkGetPhysicalDeviceProperties ) ( VkPhysicalDevice physicalDevice, void * pProperties );

static const int VK_SUCCESS			= 0;
static const int VK_MAX_PHYSICAL	= 8;

hfVulkanProbe vulkanProbe;

static bool hvkLoad( void *lib, PFN_vkGetInstanceProcAddr *outGetProc ) {
	*outGetProc = NULL;
	if ( !lib ) { return false; }
	void *proc = dlsym( lib, "vkGetInstanceProcAddr" );
	if ( !proc ) { return false; }
	*outGetProc = ( PFN_vkGetInstanceProcAddr )proc;
	return true;
}

hfVulkanProbeResult hfVulkanProbe::Probe() {
	hfVulkanProbeResult r;
	r.loaded = false;
	r.hasInstance = false;
	r.apiVersion = 0;
	r.deviceCount = 0;
	r.layerCount = 0;

	void *lib = dlopen( "libvulkan.so.1", RTLD_NOW | RTLD_LOCAL );
	if ( !lib ) {
		lib = dlopen( "libvulkan.so", RTLD_NOW | RTLD_LOCAL );
	}
	if ( !lib ) { return r; }

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = NULL;
	if ( !hvkLoad( lib, &vkGetInstanceProcAddr ) ) { dlclose( lib ); return r; }
	r.loaded = true;

	// layer count
	PFN_vkEnumerateInstanceLayerPropertiesProc vkEnumerateInstanceLayerProperties =
		( PFN_vkEnumerateInstanceLayerPropertiesProc )vkGetInstanceProcAddr( NULL, "vkEnumerateInstanceLayerProperties" );
	if ( vkEnumerateInstanceLayerProperties ) {
		unsigned int count = 0;
		vkEnumerateInstanceLayerProperties( &count, NULL );
		r.layerCount = ( int )count;
	}

	// instance
	PFN_vkCreateInstance vkCreateInstance = ( PFN_vkCreateInstance )vkGetInstanceProcAddr( NULL, "vkCreateInstance" );
	PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices =
		( PFN_vkEnumeratePhysicalDevices )vkGetInstanceProcAddr( NULL, "vkEnumeratePhysicalDevices" );
	PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties =
		( PFN_vkGetPhysicalDeviceProperties )vkGetInstanceProcAddr( NULL, "vkGetPhysicalDeviceProperties" );
	PFN_vkDestroyInstance vkDestroyInstance = ( PFN_vkDestroyInstance )vkGetInstanceProcAddr( NULL, "vkDestroyInstance" );

	if ( vkCreateInstance && vkEnumeratePhysicalDevices && vkGetPhysicalDeviceProperties ) {
		// VkApplicationInfo { sType=1(?), pApplicationName, applicationVersion, pEngineName, engineVersion, apiVersion }
		// sType VK_STRUCTURE_TYPE_APPLICATION_INFO == 0.
		char appName[] = "Hellforge";
		char engineName[] = "Hellforge alpha";
		struct { int sType; const void *pNext; const char *pApplicationName; int applicationVersion; const char *pEngineName; int engineVersion; int apiVersion; } appInfo;
		memset( &appInfo, 0, sizeof( appInfo ) );
		appInfo.sType = 0;				// VK_STRUCTURE_TYPE_APPLICATION_INFO
		appInfo.pApplicationName = appName;
		appInfo.applicationVersion = 1;
		appInfo.pEngineName = engineName;
		appInfo.engineVersion = 1;
		appInfo.apiVersion = 0x00400000 | ( 0 << 12 ) | ( 0 << 12 );	// apiVersion is u32: variant<<29|major<<22|minor<<12|patch

		struct { int sType; const void *pNext; const void *pApplicationInfo; const char **ppEnabledLayerNames; unsigned int enabledLayerCount; } createInfo;
		memset( &createInfo, 0, sizeof( createInfo ) );
		createInfo.sType = 1;			// VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
		createInfo.pApplicationInfo = &appInfo;

		VkInstance instance = NULL;
		if ( vkCreateInstance( &createInfo, NULL, &instance ) == VK_SUCCESS && instance ) {
			r.hasInstance = true;
			unsigned int count = 0;
			vkEnumeratePhysicalDevices( instance, &count, NULL );
			if ( count > VK_MAX_PHYSICAL ) { count = VK_MAX_PHYSICAL; }
			VkPhysicalDevice devices[ VK_MAX_PHYSICAL ];
			unsigned int got = count;
			if ( vkEnumeratePhysicalDevices( instance, &got, devices ) == VK_SUCCESS ) {
				r.deviceCount = ( int )got;
				for ( unsigned int i = 0; i < got && i < VK_MAX_PHYSICAL; i++ ) {
					struct {
						unsigned int apiVersion;
						unsigned int driverVersion;
						unsigned int vendorID;
						unsigned int deviceID;
						int deviceType;
						char deviceName[256];
						unsigned char pipelineCacheUUID[16];
					} props;
					memset( &props, 0, sizeof( props ) );
					vkGetPhysicalDeviceProperties( devices[i], &props );
					if ( i == 0 ) { r.apiVersion = props.apiVersion; }
					int major = ( int )( ( props.apiVersion >> 22 ) & 0x7F );
					int minor = ( int )( ( props.apiVersion >> 12 ) & 0x3FF );
					int patch = ( int )( props.apiVersion & 0xFFF );
					const char * typeName = "OTHER";
					switch ( props.deviceType ) {
						case 1: typeName = "INTEGRATED"; break;
						case 2: typeName = "DISCRETE"; break;
						case 3: typeName = "VIRTUAL"; break;
						case 4: typeName = "CPU"; break;
						default: break;
					}
					r.devices += va( "%s | v%d.%d.%d | %s\n", props.deviceName, major, minor, patch, typeName );
				}
			}
			if ( vkDestroyInstance ) { vkDestroyInstance( instance, NULL ); }
		}
	}
	dlclose( lib );
	return r;
}

hfStr hfVulkanProbe::Summary() {
	hfVulkanProbeResult r = Probe();
	if ( !r.loaded ) { return "vulkan: library not found (GL path in use)"; }
	return va( "vulkan: %s, %d device(s), api %d\n%s",
				r.loaded ? "loaded" : "missing", r.deviceCount, ( r.apiVersion >> 22 ) & 0x7F,
				r.devices.Length() ? r.devices.c_str() : "  (no usable devices)" );
}

static void hfVulkanProbe_f( const hfCmdArgs & args ) {
	hfLib::Printf( "%s", vulkanProbe.Summary().c_str() );
}

void HF_VulkanProbe_RegisterCommand() {
	static bool sRegistered = false;
	if ( sRegistered ) { return; }
	sRegistered = true;
	if ( cmdSystem ) {
		cmdSystem->AddCommand( "vulkanProbe", hfVulkanProbe_f, CMD_FL_RENDERER, "probe for a usable Vulkan driver (no device created)" );
	}
}