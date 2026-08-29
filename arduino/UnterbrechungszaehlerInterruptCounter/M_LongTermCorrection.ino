// M_LongTermCorrection.ino
// Helpers used by TimeManagement to adjust only entries recorded during the
// current provisional-time phase before the first authoritative NTP sync.

static uint32_t tmLongTermLastPhysicalIndex() {
  if (!longTermReady || longTermHeader.capacity == 0 || longTermHeader.count == 0) return 0xFFFFFFFFUL;
  return (longTermHeader.writeIndex + longTermHeader.capacity - 1) % longTermHeader.capacity;
}

static bool tmLongTermShiftPhysical(uint32_t physical, int32_t deltaSec) {
  if (!longTermReady || physical >= longTermHeader.capacity || deltaSec == 0) return false;

  File f = LittleFS.open(LONGTERM_FILE, "r+");
  if (!f) return false;

  if (!f.seek(longTermDataOffset(physical), SeekSet)) {
    f.close();
    return false;
  }

  uint32_t ts = 0;
  if (f.read((uint8_t*)&ts, sizeof(ts)) != sizeof(ts)) {
    f.close();
    return false;
  }

  int64_t shifted = (int64_t)ts + (int64_t)deltaSec;
  if (ts <= 1700000000UL || shifted <= 1700000000LL || shifted >= 4102444800LL) {
    f.close();
    return false;
  }

  uint32_t corrected = (uint32_t)shifted;
  if (!f.seek(longTermDataOffset(physical), SeekSet) ||
      f.write((const uint8_t*)&corrected, sizeof(corrected)) != sizeof(corrected)) {
    f.close();
    return false;
  }
  f.flush();
  f.close();

  // Force aggregate rebuild because count/writeIndex did not change.
  longTermCacheCount = 0xFFFFFFFFUL;
  longTermCacheWriteIndex = 0xFFFFFFFFUL;
  return true;
}
