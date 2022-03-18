#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

	static int display_info( VkInstance instance, VkPhysicalDevice* physical_devices,
							 uint32_t num_physical_devices );

	static int display2_info( VkInstance instance,
							  VkPhysicalDevice* physical_devices,
							  uint32_t num_physical_devices );

	int testDisplayInfo();


#ifdef __cplusplus
}
#endif
