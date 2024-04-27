#pragma once
#include <vector>
#include "../Utils/Debug.h"
#include <minHook.h>

class Hook
{
public:
	static void Attach(void** target, void* detour);
	static void Detach(void** target, void* detour);
};