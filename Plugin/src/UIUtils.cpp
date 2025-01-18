#include "UIUtils.h"

const char* GUI::selectionListCallback(const void* userdata, uint32_t index, char* out_buffer, uint32_t out_buffer_size)
{
	const auto* items = static_cast<const std::vector<std::string>*>(userdata);

	if (index >= items->size()) {
		return nullptr;
	}

	snprintf(out_buffer, out_buffer_size, "%s", (*items)[index].c_str());

	return out_buffer;
}
