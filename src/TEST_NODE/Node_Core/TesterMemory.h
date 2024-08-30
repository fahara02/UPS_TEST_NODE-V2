#ifndef TESTER_MEMORY_H
#define TESTER_MEMORY_H

#include <stdlib.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <cstdlib> // For malloc and free
#include <cstddef> // For size_t
#include <new> // For std::align

// Logging tag for memory allocation
const char* TAG = "MemoryAllocator";

// Function to allocate memory with specified size and alignment
static void* safeAllocateMemory(size_t size, size_t alignment)
{
	// Ensure alignment is a power of two
	if((alignment & (alignment - 1)) != 0 || alignment == 0)
	{
		ESP_LOGE(TAG, "Invalid alignment: %zu. Must be a power of two.", alignment);
		return nullptr;
	}

	// Calculate the total size needed for alignment
	size_t alignedSize = size + alignment - 1 + sizeof(void*);

	// Get available heap memory
	size_t freeHeapSize = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);

	// Check if enough memory is available
	if(freeHeapSize < alignedSize)
	{
		ESP_LOGE(TAG, "Not enough heap memory available! Requested: %zu, Free: %zu", alignedSize,
				 freeHeapSize);
		return nullptr;
	}

	// Allocate memory
	void* rawPtr = heap_caps_malloc(alignedSize, MALLOC_CAP_DEFAULT);

	if(rawPtr == nullptr)
	{
		ESP_LOGE(TAG, "Memory allocation failed! Requested: %zu, Free: %zu", alignedSize,
				 freeHeapSize);
		return nullptr;
	}

	// Adjust pointer for alignment
	void* alignedPtr = reinterpret_cast<void*>(
		(reinterpret_cast<size_t>(rawPtr) + alignment - 1 + sizeof(void*)) & ~(alignment - 1));

	// Store the original pointer just before the aligned memory
	void** alignPtr = reinterpret_cast<void**>(alignedPtr) - 1;
	*alignPtr = rawPtr;

	ESP_LOGI(TAG, "Memory allocated successfully. Address: %p, Size: %zu, Aligned Address: %p",
			 rawPtr, alignedSize, alignedPtr);

	return alignedPtr;
}

// Function to deallocate memory with specified alignment
static void safeDeallocateMemory(void* ptr)
{
	if(ptr)
	{
		// Retrieve the original pointer
		void** alignPtr = reinterpret_cast<void**>(ptr) - 1;
		void* rawPtr = *alignPtr;

		// Free the allocated memory
		heap_caps_free(rawPtr);

		ESP_LOGI("MemoryAllocator", "Memory deallocated successfully. Address: %p", rawPtr);
	}
}

#endif // TESTER_MEMORY_H
