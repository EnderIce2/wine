/*
 * Copyright 2017-2018 Roderick Colenbrander
 * Copyright 2022 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_VULKAN_DRIVER_H
#define __WINE_VULKAN_DRIVER_H

#include <stdarg.h>
#include <stddef.h>

#include <windef.h>
#include <winbase.h>

/* Base 'class' for our Vulkan dispatchable objects such as VkDevice and VkInstance.
 * This structure MUST be the first element of a dispatchable object as the ICD
 * loader depends on it. For now only contains loader_magic, but over time more common
 * functionality is expected.
 */
struct vulkan_client_object
{
    /* Special section in each dispatchable object for use by the ICD loader for
     * storing dispatch tables. The start contains a magical value '0x01CDC0DE'.
     */
    UINT64 loader_magic;
    UINT64 unix_handle;
};

#ifdef WINE_UNIX_LIB

#define WINE_VK_HOST
#include "wine/vulkan.h"

/* Wine internal vulkan driver version, needs to be bumped upon vulkan_funcs changes. */
#define WINE_VULKAN_DRIVER_VERSION 35

struct vulkan_object
{
    UINT64 host_handle;
    UINT64 client_handle;
};

#define VULKAN_OBJECT_HEADER( type, name ) \
    union { \
        struct vulkan_object obj; \
        struct { \
            union { type name; UINT64 handle; } host; \
            union { type name; UINT64 handle; } client; \
        }; \
    }

struct vulkan_instance
{
    VULKAN_OBJECT_HEADER( VkInstance, instance );
#define USE_VK_FUNC(x) PFN_ ## x p_ ## x;
    ALL_VK_INSTANCE_FUNCS
#undef USE_VK_FUNC
};

static inline struct vulkan_instance *vulkan_instance_from_handle( VkInstance handle )
{
    struct vulkan_client_object *client = (struct vulkan_client_object *)handle;
    return (struct vulkan_instance *)(UINT_PTR)client->unix_handle;
}

struct vulkan_physical_device
{
    VULKAN_OBJECT_HEADER( VkPhysicalDevice, physical_device );
    struct vulkan_instance *instance;
};

static inline struct vulkan_physical_device *vulkan_physical_device_from_handle( VkPhysicalDevice handle )
{
    struct vulkan_client_object *client = (struct vulkan_client_object *)handle;
    return (struct vulkan_physical_device *)(UINT_PTR)client->unix_handle;
}

struct vulkan_device
{
    VULKAN_OBJECT_HEADER( VkDevice, device );
    struct vulkan_physical_device *physical_device;
#define USE_VK_FUNC(x) PFN_ ## x p_ ## x;
    ALL_VK_DEVICE_FUNCS
#undef USE_VK_FUNC
};

static inline struct vulkan_device *vulkan_device_from_handle( VkDevice handle )
{
    struct vulkan_client_object *client = (struct vulkan_client_object *)handle;
    return (struct vulkan_device *)(UINT_PTR)client->unix_handle;
}

struct vulkan_queue
{
    VULKAN_OBJECT_HEADER( VkQueue, queue );
    struct vulkan_device *device;
};

static inline struct vulkan_queue *vulkan_queue_from_handle( VkQueue handle )
{
    struct vulkan_client_object *client = (struct vulkan_client_object *)handle;
    return (struct vulkan_queue *)(UINT_PTR)client->unix_handle;
}

struct vulkan_surface
{
    VULKAN_OBJECT_HEADER( VkSurfaceKHR, surface );
    struct vulkan_instance *instance;
};

static inline struct vulkan_surface *vulkan_surface_from_handle( VkSurfaceKHR handle )
{
    return (struct vulkan_surface *)(UINT_PTR)handle;
}

struct vulkan_funcs
{
    /* Vulkan global functions. These are the only calls at this point a graphics driver
     * needs to provide. Other function calls will be provided indirectly by dispatch
     * tables part of dispatchable Vulkan objects such as VkInstance or vkDevice.
     */
    PFN_vkCreateWin32SurfaceKHR p_vkCreateWin32SurfaceKHR;
    PFN_vkDestroySurfaceKHR p_vkDestroySurfaceKHR;
    PFN_vkGetDeviceProcAddr p_vkGetDeviceProcAddr;
    PFN_vkGetInstanceProcAddr p_vkGetInstanceProcAddr;
    PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR p_vkGetPhysicalDeviceWin32PresentationSupportKHR;
    VkResult (*p_vkQueuePresentKHR)(VkQueue, const VkPresentInfoKHR *, VkSurfaceKHR *surfaces);

    /* winevulkan specific functions */
    const char *(*p_get_host_surface_extension)(void);
    VkSurfaceKHR (*p_wine_get_host_surface)(VkSurfaceKHR);
    void (*p_vulkan_surface_update)(VkSurfaceKHR);
};

/* interface between win32u and the user drivers */
struct vulkan_driver_funcs
{
    VkResult (*p_vulkan_surface_create)(HWND, VkInstance, VkSurfaceKHR *, void **);
    void (*p_vulkan_surface_destroy)(HWND, void *);
    void (*p_vulkan_surface_detach)(HWND, void *);
    void (*p_vulkan_surface_update)(HWND, void *);
    void (*p_vulkan_surface_presented)(HWND, void *, VkResult);

    VkBool32 (*p_vkGetPhysicalDeviceWin32PresentationSupportKHR)(VkPhysicalDevice, uint32_t);
    const char *(*p_get_host_surface_extension)(void);
};

#endif /* WINE_UNIX_LIB */

#endif /* __WINE_VULKAN_DRIVER_H */
