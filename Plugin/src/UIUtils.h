#pragma once

#include "Utils.h"
#include <nlohmann/json.hpp>
#include "betterapi.h"
#include "ChargenUtils.h"
#include "PresetsUtils.h"
#include <functional>

namespace GUI
{
	const char* selectionListCallback(const void* userdata, uint32_t index, char* out_buffer, uint32_t out_buffer_size);
}