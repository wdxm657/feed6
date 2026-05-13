#include "factory_wifi_test_status.h"

static volatile uint8_t s_has_connect_test_result = 0U;
static volatile uint8_t s_connect_test_result = 0U;

static volatile uint8_t s_has_wifi_status = 0U;
static volatile uint8_t s_wifi_status = 0U;

void FactoryWifiTest_OnConnectTestResult(uint8_t result)
{
	s_connect_test_result = result;
	s_has_connect_test_result = 1U;
}

void FactoryWifiTest_OnWifiStatus(uint8_t status)
{
	s_wifi_status = status;
	s_has_wifi_status = 1U;
}

uint8_t FactoryWifiTest_GetConnectTestResult(uint8_t *has_value)
{
	if (has_value != 0)
	{
		*has_value = s_has_connect_test_result;
	}
	return s_connect_test_result;
}

uint8_t FactoryWifiTest_GetWifiStatus(uint8_t *has_value)
{
	if (has_value != 0)
	{
		*has_value = s_has_wifi_status;
	}
	return s_wifi_status;
}

