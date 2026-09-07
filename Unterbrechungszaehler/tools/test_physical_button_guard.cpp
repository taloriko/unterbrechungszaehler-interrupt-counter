#include <cassert>
#include <cstdint>
#include <iostream>

#include "physical_button_guard.h"

int main() {
  constexpr uint32_t cooldown = 10000U;

  PhysicalButtonGuard::State a;
  assert(PhysicalButtonGuard::accept(a, 0U, cooldown));
  assert(!PhysicalButtonGuard::accept(a, 1000U, cooldown));
  assert(!PhysicalButtonGuard::accept(a, 2000U, cooldown));
  assert(!PhysicalButtonGuard::accept(a, 9999U, cooldown));
  assert(a.suppressedCount == 3U);
  assert(PhysicalButtonGuard::accept(a, 10000U, cooldown));

  // A rejected press must not extend the window.
  PhysicalButtonGuard::State b;
  assert(PhysicalButtonGuard::accept(b, 100U, cooldown));
  assert(!PhysicalButtonGuard::accept(b, 9100U, cooldown));
  assert(PhysicalButtonGuard::accept(b, 10100U, cooldown));

  // First press after boot is always accepted, even before 10 seconds uptime.
  PhysicalButtonGuard::State c;
  assert(PhysicalButtonGuard::accept(c, 7U, cooldown));

  // Unsigned subtraction keeps the comparison correct across millis() wrap.
  PhysicalButtonGuard::State d;
  const uint32_t start = 0xFFFFF000U;
  assert(PhysicalButtonGuard::accept(d, start, cooldown));
  assert(!PhysicalButtonGuard::accept(d, static_cast<uint32_t>(start + cooldown - 1U), cooldown));
  assert(PhysicalButtonGuard::accept(d, static_cast<uint32_t>(start + cooldown), cooldown));

  std::cout << "PASS physical button anti-spam guard\n";
  return 0;
}
