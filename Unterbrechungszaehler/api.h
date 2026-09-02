#pragma once

#include <Arduino.h>

namespace Api {

String buildBootstrapJson();
String buildDeviceJson();
String buildHardwareJson();
String buildTimeJson();

}  // namespace Api
