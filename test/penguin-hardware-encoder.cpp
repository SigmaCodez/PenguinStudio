// SPDX-License-Identifier: GPL-2.0-or-later
#include "frontend/utility/PenguinHardwareEncoder.hpp"
#include <cstdio>
#include <cstdlib>

static bool cbr;
static bool cqp;
static bool disabled;
static bool missing;
static bool modified(obs_properties_t *props, obs_property_t *, obs_data_t *)
{
	auto rates = obs_properties_get(props, "rate_control");
	obs_property_list_clear(rates);
	if (cbr) {
		obs_property_list_add_string(rates, "CBR", "CBR");
		obs_property_list_item_disable(rates, 0, disabled);
	}
	if (cqp) {
		obs_property_list_add_string(rates, "CQP", "CQP");
	}
	return true;
}
static obs_properties_t *properties(void *)
{
	auto props = obs_properties_create();
	if (missing) {
		return props;
	}
	auto device = obs_properties_add_text(props, "vaapi_device", "Device", OBS_TEXT_DEFAULT);
	obs_property_set_modified_callback(device, modified);
	obs_properties_add_list(props, "rate_control", "Rate control", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	return props;
}
static void require(bool condition, const char *message)
{
	if (!condition) {
		std::fprintf(stderr, "%s\n", message);
		std::exit(1);
	}
}
static bool available()
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_default_string(settings, "vaapi_device", "test");
	OBSProperties props = properties(nullptr);
	return PenguinVAAPISupportsSimpleOutput(props, settings);
}
int main()
{
	require(!PenguinVAAPISupportsSimpleOutput(nullptr, nullptr), "missing encoder must not be selected");
	require(!available(), "no supported rates must fall back");
	cbr = true;
	require(!available(), "CBR alone cannot support quality recording");
	cbr = false;
	cqp = true;
	require(!available(), "CQP alone cannot support streaming");
	cbr = true;
	require(available(), "CBR and CQP should be available after device callback");
	disabled = true;
	require(!available(), "disabled CBR must not be selected");
	missing = true;
	require(!available(), "missing properties must fall back");
	std::puts("Hardware encoder capability checks passed");
}
