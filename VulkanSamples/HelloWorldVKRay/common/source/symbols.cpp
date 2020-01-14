// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// Copyright (c) 201x Magic Leap, Inc. (COMPANY) All Rights Reserved.
// Magic Leap, Inc. Confidential and Proprietary
//
// NOTICE: All information contained herein is, and remains the property
// of COMPANY. The intellectual and technical concepts contained herein
// are proprietary to COMPANY and may be covered by U.S. and Foreign
// Patents, patents in process, and are protected by trade secret or
// copyright law. Dissemination of this information or reproduction of
// this material is strictly forbidden unless prior written permission is
// obtained from COMPANY. Access to the source code contained herein is
// hereby forbidden to anyone except current COMPANY employees, managers
// or contractors who have executed Confidentiality and Non-disclosure
// agreements explicitly covering such access.
//
// The copyright notice above does not evidence any actual or intended
// publication or disclosure of this source code, which includes
// information that is confidential and/or proprietary, and is a trade
// secret, of COMPANY. ANY REPRODUCTION, MODIFICATION, DISTRIBUTION,
// PUBLIC PERFORMANCE, OR PUBLIC DISPLAY OF OR THROUGH USE OF THIS
// SOURCE CODE WITHOUT THE EXPRESS WRITTEN CONSENT OF COMPANY IS
// STRICTLY PROHIBITED, AND IN VIOLATION OF APPLICABLE LAWS AND
// INTERNATIONAL TREATIES. THE RECEIPT OR POSSESSION OF THIS SOURCE
// CODE AND/OR RELATED INFORMATION DOES NOT CONVEY OR IMPLY ANY RIGHTS
// TO REPRODUCE, DISCLOSE OR DISTRIBUTE ITS CONTENTS, OR TO MANUFACTURE,
// USE, OR SELL ANYTHING THAT IT MAY DESCRIBE, IN WHOLE OR IN PART.
//
// %COPYRIGHT_END%
// --------------------------------------------------------------------
// %BANNER_END%
#include <symbols.h>
#include <iostream>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace vksamples {
	namespace common {
		namespace vk {

#define VK_LOAD_GLOBAL_SYMBOL(SYM_NAME)                                                      \
  out_symbols.SYM_NAME = (PFN_##SYM_NAME)out_symbols.vkGetInstanceProcAddr(NULL, #SYM_NAME); \
  if (nullptr == out_symbols.SYM_NAME) {                                                     \
    return false;                                                                            \
  }

#define VK_LOAD_SYMBOL(SYM_NAME) \
  out_symbols.SYM_NAME = (PFN_##SYM_NAME)vkGetProcAddr(vk_handle, #SYM_NAME);

#define VK_CHECK_LOAD_SYMBOL(SYM_NAME)   \
  VK_LOAD_SYMBOL(SYM_NAME)               \
  if (nullptr == out_symbols.SYM_NAME) { \
    return false;                        \
  }

			bool LoadGlobalSymbols(GlobalSymbols &out_symbols) {
				if (!common::Library::Load("vulkan-1.dll", out_symbols.VulkanLibrary)) {
					std::cerr << "Failed to load vulkan loader1" << std::endl;
					return false;
				}
				out_symbols.vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(static_cast<HMODULE>(out_symbols.VulkanLibrary->Handle()), "vkGetInstanceProcAddr"));
				if (nullptr == out_symbols.vkGetInstanceProcAddr) {
					return false;
				}

				VK_GLOBAL_SYMBOLS(VK_LOAD_GLOBAL_SYMBOL)

					return true;
			}

			bool LoadInstanceSymbols(VkInstance const          vk_handle,
				PFN_vkGetInstanceProcAddr vkGetProcAddr,
				InstanceSymbols          &out_symbols) {
				VK_INSTANCE_SYMBOLS(VK_CHECK_LOAD_SYMBOL);
				VK_INSTANCE_EXT_SYMBOLS(VK_LOAD_SYMBOL);

				return true;
			}

			bool LoadDeviceSymbols(VkDevice const          vk_handle,
				PFN_vkGetDeviceProcAddr vkGetProcAddr,
				DeviceSymbols          &out_symbols) {
				VK_DEVICE_SYMBOLS(VK_CHECK_LOAD_SYMBOL);
				VK_DEVICE_EXT_SYMBOLS(VK_LOAD_SYMBOL);

				return true;
			}

		} // namespace vk
	} // namespace common
} // namespace vksamples