#include "TimeService.h"

#include <esp_sntp.h>
#include <sys/time.h>
#include <time.h>

#include "RtcService.h"

namespace {
TimeService* gTimeService = nullptr;

void onNtpSync(struct timeval*) {
  if (gTimeService) gTimeService->notifyNtpSynchronized();
}
}

bool TimeService::begin(RtcService* rtc) {
  rtc_ = rtc;
  gTimeService = this;
  applyTimezone();

  preferences_.begin("interrupt", true);
  primaryNtp_ = preferences_.getString("ntp1", UicConfig::DEFAULT_NTP_1);
  preferences_.end();
  if (!validNtpHost(primaryNtp_)) primaryNtp_ = UicConfig::DEFAULT_NTP_1;

  if (rtc_ && rtc_->present() && rtc_->timeValid()) {
    if (rtc_->applyToSystem()) source_ = TimeSource::Rtc;
  }

  sntp_set_time_sync_notification_cb(onNtpSync);
  startNtp();
  return true;
}

void TimeService::tick() {
  if (!ntpNotificationPending_) return;
  ntpNotificationPending_ = false;
  if (!valid()) return;

  source_ = TimeSource::Ntp;
  if (rtc_ && rtc_->present()) rtc_->writeSystemTime();
}

bool TimeService::valid() const {
  const time_t now = time(nullptr);
  return now > static_cast<time_t>(UicConfig::VALID_TIME_MIN) &&
         now < static_cast<time_t>(UicConfig::VALID_TIME_MAX);
}

uint32_t TimeService::epoch() const {
  return valid() ? static_cast<uint32_t>(time(nullptr)) : 0;
}

bool TimeService::setFromBrowser(uint32_t epochValue) {
  if (valid()) return false;
  if (epochValue <= UicConfig::VALID_TIME_MIN || epochValue >= UicConfig::VALID_TIME_MAX) return false;

  struct timeval tv = {};
  tv.tv_sec = static_cast<time_t>(epochValue);
  if (settimeofday(&tv, nullptr) != 0) return false;

  applyTimezone();
  source_ = TimeSource::Browser;
  if (rtc_ && rtc_->present()) rtc_->writeSystemTime();
  return valid();
}

bool TimeService::setPrimaryNtp(const String& host) {
  if (!validNtpHost(host)) return false;

  preferences_.begin("interrupt", false);
  const bool stored = preferences_.putString("ntp1", host) > 0;
  preferences_.end();
  if (!stored) return false;

  primaryNtp_ = host;
  startNtp();
  return true;
}

bool TimeService::validNtpHost(const String& host) const {
  if (host.length() < 1 || host.length() > 120) return false;
  for (size_t i = 0; i < host.length(); i++) {
    const char c = host[i];
    const bool ok = (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') ||
                    c == '.' || c == '-' || c == '_';
    if (!ok) return false;
  }
  return true;
}

void TimeService::notifyNtpSynchronized() {
  ntpNotificationPending_ = true;
}

String TimeService::localDate() const {
  if (!valid()) return "-";
  time_t now = time(nullptr);
  struct tm value = {};
  localtime_r(&now, &value);
  char text[16];
  snprintf(text, sizeof(text), "%02d.%02d.%04d", value.tm_mday, value.tm_mon + 1, value.tm_year + 1900);
  return String(text);
}

String TimeService::localTime() const {
  if (!valid()) return "-";
  time_t now = time(nullptr);
  struct tm value = {};
  localtime_r(&now, &value);
  char text[16];
  snprintf(text, sizeof(text), "%02d:%02d:%02d", value.tm_hour, value.tm_min, value.tm_sec);
  return String(text);
}

void TimeService::applyTimezone() {
  setenv("TZ", UicConfig::TZ_INFO, 1);
  tzset();
}

void TimeService::startNtp() {
  configTzTime(UicConfig::TZ_INFO,
               primaryNtp_.c_str(),
               UicConfig::NTP_2,
               UicConfig::NTP_3);
}
