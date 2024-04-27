#pragma once
#include <vector>
#include "../Utils/Debug.h"
#include <easyhook/easyhook.h>

class Hook
{
public:
	static void Attach(void** target, void* detour);
	static void Detach(void** target);
};