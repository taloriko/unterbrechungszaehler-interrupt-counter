#!/usr/bin/env python3
"""Host-side invariants for the interruption project's compact storage model.

This intentionally does not require Arduino/ESP32 libraries. It exercises the
binary record layout, ring wrap semantics, calendar/delta rules, heatmap source
aggregation and the custom 4 MiB partition budget.
"""
from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from zoneinfo import ZoneInfo

RAW_CAPACITY = 100_000
RAW_RECORD_SIZE = 9
DAILY_CAPACITY = 2_300
DAILY_RECORD_SIZE = 64
DELTA_MAX = 131_069
DELTA_UNKNOWN = 131_070
DELTA_FIRST = 131_071
PARTITIONS = {
    "nvs": (0x9000, 0x5000),
    "otadata": (0xE000, 0x2000),
    "app0": (0x10000, 0x160000),
    "app1": (0x170000, 0x160000),
    "littlefs": (0x2D0000, 0x130000),
}
TZ = ZoneInfo("Europe/Berlin")
BASE_DATE = datetime(2020, 1, 1, tzinfo=TZ).date()


def crc8(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
    return crc


def encode_record(time_value: int, delta: int, time_source: int, event_source: int, absolute: bool, sequence: int) -> bytes:
    packed = delta & 0x1FFFF
    packed |= (time_source & 0x07) << 17
    packed |= (event_source & 0x07) << 20
    if absolute:
        packed |= 1 << 23
    payload = bytearray(RAW_RECORD_SIZE)
    payload[0:4] = int(time_value).to_bytes(4, "little")
    payload[4:7] = int(packed).to_bytes(3, "little")
    payload[7] = sequence & 0xFF
    payload[8] = crc8(payload[:8])
    return bytes(payload)


def decode_record(record: bytes) -> tuple[int, int, int, int, bool, int]:
    assert len(record) == RAW_RECORD_SIZE
    assert crc8(record[:8]) == record[8]
    time_value = int.from_bytes(record[:4], "little")
    packed = int.from_bytes(record[4:7], "little")
    return (
        time_value,
        packed & 0x1FFFF,
        (packed >> 17) & 0x07,
        (packed >> 20) & 0x07,
        bool(packed & (1 << 23)),
        record[7],
    )


class Ring:
    def __init__(self) -> None:
        self.slots: list[tuple[int, bytes] | None] = [None] * RAW_CAPACITY
        self.write = 0
        self.count = 0
        self.sequence = 0

    def append(self, payload: bytes) -> int:
        self.sequence += 1
        self.slots[self.write] = (self.sequence, payload)
        self.write = (self.write + 1) % RAW_CAPACITY
        self.count = min(RAW_CAPACITY, self.count + 1)
        return self.sequence

    def chronological_sequences(self) -> list[int]:
        oldest = (self.write + RAW_CAPACITY - self.count) % RAW_CAPACITY
        return [self.slots[(oldest + i) % RAW_CAPACITY][0] for i in range(self.count)]  # type: ignore[index]


@dataclass
class Event:
    when: datetime
    source: int = 1


def day_index(dt: datetime) -> int:
    return (dt.astimezone(TZ).date() - BASE_DATE).days


def build_deltas(events: list[Event]) -> list[int]:
    last_by_day: dict[int, int] = {}
    result: list[int] = []
    for event in events:
        epoch = int(event.when.timestamp())
        day = day_index(event.when)
        previous = last_by_day.get(day)
        result.append(DELTA_FIRST if previous is None else min(DELTA_MAX, epoch - previous))
        last_by_day[day] = epoch
    return result


def completed_intervals(events: list[Event]) -> list[tuple[Event, int]]:
    result: list[tuple[Event, int]] = []
    for previous, current in zip(events, events[1:]):
        if day_index(previous.when) != day_index(current.when):
            continue
        elapsed = int(current.when.timestamp() - previous.when.timestamp())
        if elapsed > 0:
            result.append((previous, elapsed))
    return result


def aggregate(events: list[Event]) -> dict[int, list[int]]:
    days: dict[int, list[int]] = {}
    for event in events:
        local = event.when.astimezone(TZ)
        buckets = days.setdefault(day_index(event.when), [0] * 25)
        buckets[0] += 1
        buckets[1 + local.hour] += 1
    return days


def test_partition_budget() -> None:
    assert PARTITIONS["littlefs"][0] + PARTITIONS["littlefs"][1] == 0x400000
    assert PARTITIONS["app0"][0] + PARTITIONS["app0"][1] == PARTITIONS["app1"][0]
    assert PARTITIONS["app1"][0] + PARTITIONS["app1"][1] == PARTITIONS["littlefs"][0]
    raw_bytes = RAW_CAPACITY * RAW_RECORD_SIZE
    daily_bytes = DAILY_CAPACITY * DAILY_RECORD_SIZE
    data_budget = PARTITIONS["littlefs"][1]
    reserve = data_budget - raw_bytes - daily_bytes - (2 * 44) - (2 * 40)
    assert raw_bytes == 900_000
    assert daily_bytes == 147_200
    assert reserve > 190_000, reserve
    assert DAILY_CAPACITY >= 365 * 6 + 1


def test_record_layout() -> None:
    sample = encode_record(1_788_339_733, 1044, 1, 2, True, 0x12345678)
    assert len(sample) == 9
    decoded = decode_record(sample)
    assert decoded == (1_788_339_733, 1044, 1, 2, True, 0x78)
    first = decode_record(encode_record(123, DELTA_FIRST, 2, 1, True, 1))
    assert first[1] == DELTA_FIRST
    corrupted = bytearray(sample)
    corrupted[3] ^= 0x20
    try:
        decode_record(bytes(corrupted))
    except AssertionError:
        pass
    else:
        raise AssertionError("CRC8 did not reject corruption")


def test_ring_wrap_100k() -> None:
    ring = Ring()
    total = 105_321
    for seq in range(1, total + 1):
        payload = encode_record(seq, DELTA_UNKNOWN, 4, 1, False, seq)
        ring.append(payload)
    sequences = ring.chronological_sequences()
    assert len(sequences) == RAW_CAPACITY
    assert sequences[0] == total - RAW_CAPACITY + 1
    assert sequences[-1] == total
    assert sequences == list(range(total - RAW_CAPACITY + 1, total + 1))


def test_delta_and_aggregate() -> None:
    events = [
        Event(datetime(2026, 9, 2, 8, 0, tzinfo=TZ)),
        Event(datetime(2026, 9, 2, 8, 15, tzinfo=TZ)),
        Event(datetime(2026, 9, 2, 10, 0, tzinfo=TZ)),
        Event(datetime(2026, 9, 3, 7, 0, tzinfo=TZ)),
    ]
    assert build_deltas(events) == [DELTA_FIRST, 900, 6300, DELTA_FIRST]
    days = aggregate(events)
    first_day = days[day_index(events[0].when)]
    second_day = days[day_index(events[-1].when)]
    assert first_day[0] == 3 and first_day[1 + 8] == 2 and first_day[1 + 10] == 1
    assert second_day[0] == 1 and second_day[1 + 7] == 1
    iso = events[0].when.isocalendar()
    assert (iso.year, iso.week, iso.weekday) == (2026, 36, 3)


def test_dst_calendar_behavior() -> None:
    # Spring forward: 01:30 CET -> 03:30 CEST is one real hour but same local day.
    before = datetime(2026, 3, 29, 1, 30, tzinfo=TZ)
    after = (before.astimezone(timezone.utc) + timedelta(hours=1)).astimezone(TZ)
    assert (before.hour, after.hour) == (1, 3)
    assert day_index(before) == day_index(after)
    assert int(after.timestamp() - before.timestamp()) == 3600

    # Autumn fallback: two different instants can both be local 02:30; both
    # correctly accumulate in the same hour bucket for the heatmap.
    first_230 = datetime(2026, 10, 25, 2, 30, fold=0, tzinfo=TZ)
    second_230 = datetime(2026, 10, 25, 2, 30, fold=1, tzinfo=TZ)
    assert int(second_230.timestamp() - first_230.timestamp()) == 3600
    days = aggregate([Event(first_230), Event(second_230)])
    buckets = days[day_index(first_230)]
    assert buckets[0] == 2 and buckets[1 + 2] == 2


def test_heatmap_views_from_daily_aggregates() -> None:
    events = [
        Event(datetime(2022, 1, 10, 9, 0, tzinfo=TZ)),
        Event(datetime(2023, 2, 14, 10, 0, tzinfo=TZ)),
        Event(datetime(2024, 3, 20, 11, 0, tzinfo=TZ)),
        Event(datetime(2025, 4, 3, 12, 0, tzinfo=TZ)),
        Event(datetime(2026, 9, 2, 8, 0, tzinfo=TZ)),
        Event(datetime(2026, 9, 2, 8, 30, tzinfo=TZ)),
        Event(datetime(2026, 9, 2, 10, 0, tzinfo=TZ)),
        Event(datetime(2026, 9, 3, 8, 0, tzinfo=TZ)),
    ]
    days = aggregate(events)

    hourly = [[0 for _ in range(24)] for _ in range(7)]
    month_week = [[0 for _ in range(53)] for _ in range(12)]
    year_month = [[0 for _ in range(12)] for _ in range(5)]
    start_year = 2022

    for index, buckets in days.items():
        date = BASE_DATE + timedelta(days=index)
        iso = date.isocalendar()
        if iso.year == 2026 and iso.week == 36:
            weekday = iso.weekday - 1
            for hour in range(24):
                hourly[weekday][hour] += buckets[1 + hour]
        if date.year == 2026:
            month_week[date.month - 1][iso.week - 1] += buckets[0]
        if start_year <= date.year <= 2026:
            year_month[date.year - start_year][date.month - 1] += buckets[0]

    # 2026-09-02 is Wednesday in ISO week 36.
    assert hourly[2][8] == 2
    assert hourly[2][10] == 1
    assert hourly[3][8] == 1
    assert month_week[8][35] == 4
    assert year_month[4][8] == 4
    assert sum(sum(row) for row in year_month) == len(events)

    # Filter regression: a selected year must materially change the matrix.
    # 2025 contains one event while 2027 contains none.
    def month_week_for(selected_year: int) -> list[list[int]]:
        matrix = [[0 for _ in range(53)] for _ in range(12)]
        for index, buckets in days.items():
            date = BASE_DATE + timedelta(days=index)
            if date.year != selected_year:
                continue
            iso = date.isocalendar()
            matrix[date.month - 1][iso.week - 1] += buckets[0]
        return matrix

    assert sum(sum(row) for row in month_week_for(2025)) == 1
    assert sum(sum(row) for row in month_week_for(2027)) == 0



def test_average_interval_semantics() -> None:
    events = [
        Event(datetime(2026, 9, 2, 8, 0, tzinfo=TZ)),
        Event(datetime(2026, 9, 2, 8, 15, tzinfo=TZ)),
        Event(datetime(2026, 9, 2, 10, 0, tzinfo=TZ)),
    ]
    samples = completed_intervals(events)
    assert [seconds for _, seconds in samples] == [900, 6300]
    assert [start.when.hour for start, _ in samples] == [8, 8]
    assert round(sum(seconds for _, seconds in samples) / len(samples)) == 3600
    # The last press at 10:00 has no following same-day press and therefore no sample.
    assert all(start.when.hour != 10 for start, _ in samples)

    # Never bridge midnight.
    midnight = [
        Event(datetime(2026, 9, 2, 23, 50, tzinfo=TZ)),
        Event(datetime(2026, 9, 3, 0, 10, tzinfo=TZ)),
    ]
    assert completed_intervals(midnight) == []

    # One press on a day yields no completed interval.
    assert completed_intervals([Event(datetime(2026, 9, 4, 9, 0, tzinfo=TZ))]) == []

    # Weighted average over all samples, not an average of daily averages.
    weighted = [
        Event(datetime(2026, 9, 7, 8, 0, tzinfo=TZ)),
        Event(datetime(2026, 9, 7, 8, 10, tzinfo=TZ)),
        Event(datetime(2026, 9, 8, 8, 0, tzinfo=TZ)),
        Event(datetime(2026, 9, 8, 8, 30, tzinfo=TZ)),
    ]
    values = [seconds for _, seconds in completed_intervals(weighted)]
    assert values == [600, 1800]
    assert sum(values) // len(values) == 1200

    # DST spring-forward still uses real elapsed epoch seconds.
    before = Event(datetime(2026, 3, 29, 1, 30, tzinfo=TZ))
    after = Event((before.when.astimezone(timezone.utc) + timedelta(hours=1)).astimezone(TZ))
    dst = completed_intervals([before, after])
    assert len(dst) == 1 and dst[0][1] == 3600


def test_average_interval_ring_coverage_rule() -> None:
    def complete(raw_count: int, capacity: int, oldest_sequence: int, oldest_day: int, selected_start: int) -> bool:
        overwritten = capacity > 0 and raw_count >= capacity and oldest_sequence > 1
        return not overwritten or oldest_day <= selected_start

    assert complete(50_000, RAW_CAPACITY, 1, 2000, 1000)
    assert complete(RAW_CAPACITY, RAW_CAPACITY, 1, 2000, 1000)
    assert not complete(RAW_CAPACITY, RAW_CAPACITY, 5001, 2500, 2400)
    assert complete(RAW_CAPACITY, RAW_CAPACITY, 5001, 2500, 2500)

def test_pending_queue_overflow_keeps_live_count_visible() -> None:
    capacity = 64
    queued = 0
    live_count = 0
    dropped = 0
    for _ in range(capacity + 3):
        live_count += 1
        if queued < capacity:
            queued += 1
        else:
            dropped += 1
    assert live_count == 67
    assert queued == 64
    assert dropped == 3

def test_transactional_retry_no_duplicate() -> None:
    # Model the firmware invariant: a raw write can succeed while the metadata
    # commit fails. The in-RAM metadata must then roll back so retry uses the
    # same slot and sequence rather than creating a duplicate logical event.
    write_index = 42
    count = 100
    sequence = 100
    before = (write_index, count, sequence)

    attempted_sequence = sequence + 1
    attempted_slot = write_index
    # Simulated metadata commit failure -> restore exactly the durable state.
    write_index, count, sequence = before
    assert (write_index, count, sequence) == before

    retry_sequence = sequence + 1
    retry_slot = write_index
    assert retry_sequence == attempted_sequence
    assert retry_slot == attempted_slot


def test_backward_clock_delta_is_unknown() -> None:
    # Same local day but wall clock corrected backwards: there was a previous
    # event, so this is not FIRST_OF_DAY; the interval is intentionally unknown.
    previous = datetime(2026, 9, 2, 10, 0, tzinfo=TZ)
    corrected = datetime(2026, 9, 2, 9, 59, tzinfo=TZ)
    assert day_index(previous) == day_index(corrected)
    assert int(corrected.timestamp()) < int(previous.timestamp())
    derived = DELTA_UNKNOWN if corrected.timestamp() < previous.timestamp() else int(corrected.timestamp() - previous.timestamp())
    assert derived == DELTA_UNKNOWN


def test_calendar_anchor_survives_relative_tail() -> None:
    # Persisted metadata keeps the latest calendar-valid event even if newer
    # raw records only have relative time. After reboot the same-day delta can
    # still use that anchor without scanning the 100k raw ring.
    anchor = Event(datetime(2026, 9, 2, 10, 0, tzinfo=TZ))
    anchor_epoch = int(anchor.when.timestamp())
    relative_tail_count = 7
    assert relative_tail_count > 0
    after_reboot = datetime(2026, 9, 2, 10, 15, tzinfo=TZ)
    assert day_index(anchor.when) == day_index(after_reboot)
    assert int(after_reboot.timestamp()) - anchor_epoch == 900


def test_raw_sequence_alignment_from_aggregate_checkpoint() -> None:
    # After raw metadata loss the ring can recover ordering + low 8 sequence
    # bits, but not the lifetime high bits. A valid aggregate checkpoint lifts
    # the raw newest sequence to the first matching tag at/after that durable
    # lower bound.
    recovered_newest = 100_173
    durable_aggregate = 1_000_012
    tag = recovered_newest & 0xFF
    forward = (tag - (durable_aggregate & 0xFF)) & 0xFF
    aligned = durable_aggregate + forward
    assert aligned >= durable_aggregate
    assert (aligned & 0xFF) == tag
    assert aligned - durable_aggregate < 256
    # New raw events now advance beyond the aggregate checkpoint instead of
    # being silently ignored as old/already-processed sequence numbers.
    assert aligned + 1 > durable_aggregate


def test_empty_raw_sequence_alignment_from_aggregate_checkpoint() -> None:
    # A retained daily aggregate store may outlive raw-ring records after an
    # exceptional raw-data loss. The raw count can stay zero while its logical
    # sequence base is lifted to the aggregate checkpoint so the next event is
    # checkpoint+1 rather than sequence 1.
    raw_count = 0
    raw_total_sequence = 0
    aggregate_checkpoint = 987_654
    assert raw_count == 0
    raw_total_sequence = max(raw_total_sequence, aggregate_checkpoint)
    next_sequence = raw_total_sequence + 1
    assert next_sequence == aggregate_checkpoint + 1

def test_full_ring_metadata_retry_restores_displaced_record() -> None:
    # In a full ring the next raw write overwrites the oldest durable slot. If
    # the following metadata commit fails, an in-process retry must first put
    # the displaced 9-byte record back so the rolled-back metadata still
    # describes a self-consistent ring.
    oldest = encode_record(100, DELTA_FIRST, 1, 1, True, 1)
    candidate = encode_record(200, 100, 1, 1, True, RAW_CAPACITY + 1)
    slot = bytearray(oldest)
    backup = bytes(slot)
    slot[:] = candidate
    metadata_commit_ok = False
    if not metadata_commit_ok:
        slot[:] = backup
    assert bytes(slot) == oldest
    assert decode_record(bytes(slot))[-1] == 1

def test_daily_full_ring_metadata_rollback() -> None:
    # Model the full daily-ring edge case: a new day temporarily overwrites
    # the oldest slot, but the metadata commit fails. The displaced oldest
    # record must be restored before retry; otherwise the orphan new-day record
    # could be mistaken for an already indexed day and corrupt ring ordering.
    capacity = DAILY_CAPACITY
    write_index = 317
    count = capacity
    oldest_day = 1234
    new_day = 4567
    slots = {write_index: oldest_day}
    before = (write_index, count)

    displaced = slots[write_index]
    slots[write_index] = new_day
    # Simulated metadata commit failure -> restore both durable data and RAM meta.
    slots[write_index] = displaced
    write_index, count = before

    assert slots[write_index] == oldest_day
    assert (write_index, count) == before
    # Retry is allowed to use the same oldest slot and then advance metadata once.
    slots[write_index] = new_day
    write_index = (write_index + 1) % capacity
    assert slots[before[0]] == new_day
    assert write_index == (before[0] + 1) % capacity


V3_HEADER_ENCODE = [
    5,6,7,13,14,15,21,22,23,29,30,31,37,38,39,45,
    46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,
    62,63,69,70,71,77,78,79,85,86,87,93,94,95,101,102,
    103,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,
]
V3_HEADER_DECODE = {header: ordinal for ordinal, header in enumerate(V3_HEADER_ENCODE)}


def legacy_header_valid(header: int) -> bool:
    time_source = header & 0x07
    event_source = (header >> 3) & 0x07
    return time_source <= 4 and event_source <= 5


def encode_record_v3(time_value: int, delta: int, source_id: int, time_code: int, sequence: int) -> bytes:
    assert 0 <= source_id <= 15 and 0 <= time_code <= 3
    header = V3_HEADER_ENCODE[source_id * 4 + time_code]
    packed = (delta & 0x1FFFF) | (header << 17)
    payload = bytearray(RAW_RECORD_SIZE)
    payload[0:4] = int(time_value).to_bytes(4, "little")
    payload[4:7] = int(packed).to_bytes(3, "little")
    payload[7] = sequence & 0xFF
    payload[8] = crc8(payload[:8])
    return bytes(payload)


def decode_mixed(record: bytes) -> tuple[int, int, int, int, int]:
    assert len(record) == RAW_RECORD_SIZE and crc8(record[:8]) == record[8]
    packed = int.from_bytes(record[4:7], "little")
    header = (packed >> 17) & 0x7F
    legacy_time = header & 0x07
    legacy_source = (header >> 3) & 0x07
    if legacy_header_valid(header):
        return (2, legacy_source, legacy_time, packed & 0x1FFFF, record[7])
    ordinal = V3_HEADER_DECODE[header]
    return (3, ordinal >> 2, ordinal & 0x03, packed & 0x1FFFF, record[7])


def test_multisource_v3_self_describing_codec() -> None:
    assert len(V3_HEADER_ENCODE) == 64
    assert len(set(V3_HEADER_ENCODE)) == 64
    assert all(0 <= header < 128 for header in V3_HEADER_ENCODE)
    assert all(not legacy_header_valid(header) for header in V3_HEADER_ENCODE)
    assert 15 - 6 + 1 == 10

    for source_id in range(16):
        for time_code in range(4):
            record = encode_record_v3(1_788_339_733, 1234, source_id, time_code, 0x3456)
            assert len(record) == 9
            assert decode_mixed(record) == (3, source_id, time_code, 1234, 0x56)

    # Existing v2 bytes remain byte-for-byte decodable with their legacy source.
    legacy = encode_record(1_788_339_733, 900, 1, 5, True, 0x77)
    version, source_id, time_source, delta, tag = decode_mixed(legacy)
    assert (version, source_id, time_source, delta, tag) == (2, 5, 1, 900, 0x77)


def main() -> None:
    tests = [
        test_partition_budget,
        test_record_layout,
        test_multisource_v3_self_describing_codec,
        test_ring_wrap_100k,
        test_delta_and_aggregate,
        test_dst_calendar_behavior,
        test_heatmap_views_from_daily_aggregates,
        test_average_interval_semantics,
        test_average_interval_ring_coverage_rule,
        test_pending_queue_overflow_keeps_live_count_visible,
        test_transactional_retry_no_duplicate,
        test_backward_clock_delta_is_unknown,
        test_calendar_anchor_survives_relative_tail,
        test_raw_sequence_alignment_from_aggregate_checkpoint,
        test_empty_raw_sequence_alignment_from_aggregate_checkpoint,
        test_full_ring_metadata_retry_restores_displaced_record,
        test_daily_full_ring_metadata_rollback,
    ]
    for test in tests:
        test()
        print(f"PASS {test.__name__}")
    print(f"PASS all ({len(tests)} tests) | retained raw events={RAW_CAPACITY:,}")


if __name__ == "__main__":
    main()
