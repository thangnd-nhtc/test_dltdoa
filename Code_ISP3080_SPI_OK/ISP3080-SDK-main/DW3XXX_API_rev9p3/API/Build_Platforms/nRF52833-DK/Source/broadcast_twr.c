#include "broadcast_twr.h"
#include "DW3000.h"
#include "main.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Active table chứa danh sách Tag ID (5 bytes mỗi Tag). BLE chỉ update table,
// scheduler TWR sẽ đo round-robin, không xóa tag sau mỗi lần đo.
#define BCAST_TWR_MAX_TAGS 200
#define BCAST_TWR_RSSI_THRESHOLD -85
#define BCAST_TWR_TAG_EXPIRE_TICKS APP_TIMER_TICKS(10000)
#define BCAST_TWR_MIN_MEASURE_INTERVAL_TICKS APP_TIMER_TICKS(100)

#include "app_timer.h"

typedef struct
{
  uint8_t id[5];
  uint32_t last_seen_ticks;
  uint32_t last_measure_ticks;
  uint16_t success_count;
  uint16_t fail_count;
  bool active;
} bcast_tag_entry_t;

static bcast_tag_entry_t m_tags[BCAST_TWR_MAX_TAGS];
static uint16_t m_tag_count = 0;
static bool m_is_enabled = false;
static int16_t m_current_tag_idx = -1;
static int16_t m_rr_index = -1;

static bool bcast_twr_id_is_empty(const uint8_t *id)
{
  for (uint8_t i = 0; i < 5; i++)
  {
    if (id[i] != 0x00 && id[i] != 0xFF)
      return false;
  }
  return true;
}

void bcast_twr_init(void)
{
  memset(m_tags, 0, sizeof(m_tags));
  m_tag_count = 0;
  m_current_tag_idx = -1;
  m_rr_index = -1;
  m_is_enabled = false;
}

void bcast_twr_enable(void)
{
  if (!m_is_enabled)
  {
    printf("[BCAST_TWR] Enabled\n");
  }
  m_is_enabled = true;
}

void bcast_twr_disable(void)
{
  if (m_is_enabled)
  {
    printf("[BCAST_TWR] Disabled\n");
  }
  m_is_enabled = false;
  m_tag_count = 0;
  m_current_tag_idx = -1;
  m_rr_index = -1;
  memset(m_tags, 0, sizeof(m_tags));
}

bool bcast_twr_is_enabled(void) { return m_is_enabled; }

void bcast_twr_on_ack_received(const uint8_t *p_tag_id_raw, int8_t rssi)
{
  if (!m_is_enabled || p_tag_id_raw == NULL)
    return;

  if (rssi < BCAST_TWR_RSSI_THRESHOLD || bcast_twr_id_is_empty(p_tag_id_raw))
    return;

  uint32_t now = app_timer_cnt_get();

  // Nếu tag đã có trong active table, chỉ update last_seen.
  for (uint16_t i = 0; i < m_tag_count; i++)
  {
    if (memcmp(m_tags[i].id, p_tag_id_raw, 5) == 0)
    {
      m_tags[i].last_seen_ticks = now;
      m_tags[i].active = true;
      return;
    }
  }

  // Thêm tag mới vào table nếu còn chỗ.
  if (m_tag_count < BCAST_TWR_MAX_TAGS)
  {
    bcast_tag_entry_t *tag = &m_tags[m_tag_count];
    memset(tag, 0, sizeof(*tag));
    memcpy(tag->id, p_tag_id_raw, 5);
    tag->last_seen_ticks = now;
    tag->last_measure_ticks = 0;
    tag->active = true;
    m_tag_count++;
  }
}

bool bcast_twr_next_tag(uint8_t *p_tag_id_out)
{
  if (!m_is_enabled || m_tag_count == 0 || p_tag_id_out == NULL)
    return false;

  uint32_t now = app_timer_cnt_get();

  for (uint16_t tries = 0; tries < m_tag_count; tries++)
  {
    m_rr_index++;
    if (m_rr_index >= (int16_t)m_tag_count)
      m_rr_index = 0;

    bcast_tag_entry_t *tag = &m_tags[m_rr_index];

    if (!tag->active)
      continue;

    uint32_t seen_diff = app_timer_cnt_diff_compute(now, tag->last_seen_ticks);
    if (seen_diff > BCAST_TWR_TAG_EXPIRE_TICKS)
    {
      tag->active = false;
      continue;
    }

    if (tag->last_measure_ticks != 0)
    {
      uint32_t measure_diff =
          app_timer_cnt_diff_compute(now, tag->last_measure_ticks);
      if (measure_diff < BCAST_TWR_MIN_MEASURE_INTERVAL_TICKS)
        continue;
    }

    memcpy(p_tag_id_out, tag->id, 5);
    tag->last_measure_ticks = now;
    m_current_tag_idx = m_rr_index;
    return true;
  }

  m_current_tag_idx = -1;
  return false;
}

void bcast_twr_finish_current(bool success)
{
  if (!m_is_enabled || m_current_tag_idx < 0 ||
      m_current_tag_idx >= (int16_t)m_tag_count)
    return;

  bcast_tag_entry_t *tag = &m_tags[m_current_tag_idx];
  if (success)
    tag->success_count++;
  else
    tag->fail_count++;

  m_current_tag_idx = -1;
}

void bcast_twr_remove_current(void)
{
  // Giữ API cũ cho main.c/đoạn code khác: đo xong không xóa tag khỏi table nữa.
  bcast_twr_finish_current(true);
}

void bcast_twr_process(void)
{
  if (!m_is_enabled || m_tag_count == 0)
    return;

  uint32_t now = app_timer_cnt_get();

  // Cleanup định kỳ: xóa hẳn các tag lâu không còn BLE seen để table không đầy.
  for (uint16_t i = 0; i < m_tag_count;)
  {
    uint32_t seen_diff = app_timer_cnt_diff_compute(now, m_tags[i].last_seen_ticks);
    if (!m_tags[i].active || seen_diff > BCAST_TWR_TAG_EXPIRE_TICKS)
    {
      if (i == (uint16_t)m_current_tag_idx)
        m_current_tag_idx = -1;
      if (i <= (uint16_t)m_rr_index && m_rr_index > 0)
        m_rr_index--;

      if (i < m_tag_count - 1)
      {
        memmove(&m_tags[i], &m_tags[i + 1],
                sizeof(bcast_tag_entry_t) * (m_tag_count - i - 1));
      }
      m_tag_count--;
      continue;
    }
    i++;
  }

  if (m_tag_count == 0)
  {
    m_current_tag_idx = -1;
    m_rr_index = -1;
  }
}
