#ifndef __FACTORY_WIFI_TEST_STATUS_H__
#define __FACTORY_WIFI_TEST_STATUS_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

void FactoryWifiTest_OnConnectTestResult(uint8_t result);
void FactoryWifiTest_OnWifiStatus(uint8_t status);

uint8_t FactoryWifiTest_GetConnectTestResult(uint8_t *has_value);
uint8_t FactoryWifiTest_GetWifiStatus(uint8_t *has_value);

#ifdef __cplusplus
}
#endif

#endif /* __FACTORY_WIFI_TEST_STATUS_H__ */

