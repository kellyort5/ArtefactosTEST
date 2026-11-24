#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/ble_sm.h"
#include "host/ble_store.h"
#include "include/ble_storage.h"
#include "include/sentinel.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "store/config/ble_store_config.h"
extern "C" void ble_store_config_init(void);
#include <atomic>
#include <cstring>
#if !CONFIG_BT_NIMBLE_SM_SC || CONFIG_BT_NIMBLE_SM_SC_DEBUG_KEYS
#error Sentinel requires production LE Secure Connections support
#endif
namespace sentinel {
static const ble_uuid128_t service =
    BLE_UUID128_INIT(0x00, 0x00, 0x1f, 0x1f, 0xb5, 0x9d, 0x6d, 0x9e, 0x43, 0x4f,
                     0x64, 0x6d, 0x01, 0x00, 0x0d, 0xa9);
static const ble_uuid128_t meta_uuid =
    BLE_UUID128_INIT(0x00, 0x00, 0x1f, 0x1f, 0xb5, 0x9d, 0x6d, 0x9e, 0x43, 0x4f,
                     0x64, 0x6d, 0x02, 0x00, 0x0d, 0xa9);
static const ble_uuid128_t input_uuid =
    BLE_UUID128_INIT(0x00, 0x00, 0x1f, 0x1f, 0xb5, 0x9d, 0x6d, 0x9e, 0x43, 0x4f,
                     0x64, 0x6d, 0x03, 0x00, 0x0d, 0xa9);
static const ble_uuid128_t output_uuid =
    BLE_UUID128_INIT(0x00, 0x00, 0x1f, 0x1f, 0xb5, 0x9d, 0x6d, 0x9e, 0x43, 0x4f,
                     0x64, 0x6d, 0x04, 0x00, 0x0d, 0xa9);
static std::atomic<uint16_t> connection{BLE_HS_CONN_HANDLE_NONE};
static std::atomic<uint32_t> generation{1};
static std::atomic<uint64_t> window_until{0};
static std::atomic<bool> subscribed{false}, host_ready{false},
    new_client{false};
static int secure_store_write(int type, const union ble_store_value *value) {
  if (type == BLE_STORE_OBJ_TYPE_OUR_SEC ||
      type == BLE_STORE_OBJ_TYPE_PEER_SEC) {
    if (!value->sec.authenticated || !value->sec.sc ||
        value->sec.key_size != 16)
      return BLE_HS_EAUTHEN;
    if (new_client && monotonic_ms() >= window_until)
      return BLE_HS_EAUTHEN;
  }
  return ble_store_config_write(type, value);
}
static Frame incoming;
static char metadata_read[257] = {};
static uint32_t metadata_session = 0;
static uint16_t output_handle;
static uint8_t address_type;
struct Tx {
  char bytes[MaxMessage + 1];
  uint16_t length;
  uint32_t session;
};
static QueueHandle_t outgoing;
static SemaphoreHandle_t confirmation;
static std::atomic<int> indication_status{BLE_HS_EUNKNOWN};
static int gap(struct ble_gap_event *, void *);
static bool known(uint16_t handle) {
  ble_gap_conn_desc d = {};
  if (ble_gap_conn_find(handle, &d))
    return false;
  ble_addr_t peers[CONFIG_BT_NIMBLE_MAX_BONDS];
  int count = 0;
  if (ble_store_util_bonded_peers(peers, &count, CONFIG_BT_NIMBLE_MAX_BONDS))
    return false;
  for (int i = 0; i < count; ++i)
    if (!ble_addr_cmp(&d.peer_id_addr, &peers[i]))
      return true;
  return false;
}
static bool secure(uint16_t handle) {
  ble_gap_conn_desc d = {};
  return handle == connection && !ble_gap_conn_find(handle, &d) &&
         d.sec_state.encrypted && d.sec_state.authenticated &&
         d.sec_state.bonded && d.sec_state.key_size == 16 && known(handle);
}
static void advertise() {
  if (!host_ready || connection != BLE_HS_CONN_HANDLE_NONE)
    return;
  ble_hs_adv_fields fields = {};
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
  fields.uuids128 = const_cast<ble_uuid128_t *>(&service);
  fields.num_uuids128 = 1;
  fields.uuids128_is_complete = 1;
  if (ble_gap_adv_set_fields(&fields))
    return;
  ble_gap_adv_params params = {};
  params.conn_mode = BLE_GAP_CONN_MODE_UND;
  params.disc_mode = BLE_GAP_DISC_MODE_GEN;
  ble_gap_adv_start(address_type, nullptr, BLE_HS_FOREVER, &params, gap,
                    nullptr);
}
static int access(uint16_t conn, uint16_t, struct ble_gatt_access_ctxt *ctx,
                  void *arg) {
  if (!secure(conn))
    return BLE_ATT_ERR_INSUFFICIENT_AUTHEN;
  if (arg == reinterpret_cast<void *>(1)) {
    if (ctx->offset == 0 || metadata_session != generation) {
      if (!metadata(metadata_read, sizeof(metadata_read)))
        return BLE_ATT_ERR_UNLIKELY;
      metadata_session = generation.load();
    }
    return os_mbuf_append(ctx->om, metadata_read, strlen(metadata_read))
               ? BLE_ATT_ERR_INSUFFICIENT_RES
               : 0;
  }
  if (arg != reinterpret_cast<void *>(2) ||
      ctx->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
    return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
  uint8_t packet[517];
  uint16_t n = 0;
  if (ble_hs_mbuf_to_flat(ctx->om, packet, sizeof(packet), &n)) {
    incoming.reset();
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  }
  int result = incoming.push(packet, n, monotonic_ms());
  wipe(packet, sizeof(packet));
  if (result < 0)
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  if (result == 1) {
    bool queued =
        submit(Channel::Ble, reinterpret_cast<const char *>(incoming.bytes),
               incoming.used, true, false, generation.load());
    incoming.reset();
    if (!queued)
      return BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  return 0;
}
static ble_gatt_chr_def chars[4];
static ble_gatt_svc_def services[2];
static void sync() {
  if (ble_hs_id_infer_auto(0, &address_type))
    return;
  host_ready = true;
  advertise();
}
static int store_status(struct ble_store_status_event *, void *) {
  return BLE_HS_ENOMEM;
} // No automatic eviction of authorized bonds.
static int gap(struct ble_gap_event *e, void *) {
  switch (e->type) {
  case BLE_GAP_EVENT_CONNECT:
    if (e->connect.status) {
      advertise();
      break;
    }
    if (connection != BLE_HS_CONN_HANDLE_NONE) {
      ble_gap_terminate(e->connect.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
      break;
    }
    connection = e->connect.conn_handle;
    generation.fetch_add(1);
    incoming.reset();
    subscribed = false;
    new_client = !known(connection);
    if (new_client && monotonic_ms() >= window_until) {
      ble_gap_terminate(connection, BLE_ERR_REM_USER_CONN_TERM);
      break;
    }
    post({Kind::BleUp});
    if (ble_gap_security_initiate(connection))
      ble_gap_terminate(connection, BLE_ERR_AUTH_FAIL);
    break;
  case BLE_GAP_EVENT_DISCONNECT:
    if (e->disconnect.conn.conn_handle != connection)
      break;
    connection = BLE_HS_CONN_HANDLE_NONE;
    generation.fetch_add(1);
    incoming.reset();
    subscribed = false;
    xSemaphoreGive(confirmation);
    post({Kind::BleDown});
    advertise();
    break;
  case BLE_GAP_EVENT_ADV_COMPLETE:
    advertise();
    break;
  case BLE_GAP_EVENT_ENC_CHANGE:
    if (new_client && monotonic_ms() >= window_until) {
      ble_gap_conn_desc d = {};
      if (!ble_gap_conn_find(e->enc_change.conn_handle, &d))
        ble_gap_unpair(&d.peer_id_addr);
      ble_gap_terminate(e->enc_change.conn_handle, BLE_ERR_AUTH_FAIL);
    } else if (e->enc_change.status || !secure(e->enc_change.conn_handle))
      ble_gap_terminate(e->enc_change.conn_handle, BLE_ERR_AUTH_FAIL);
    else
      new_client = false;
    break;
  case BLE_GAP_EVENT_REPEAT_PAIRING:
    return BLE_GAP_REPEAT_PAIRING_IGNORE;
  case BLE_GAP_EVENT_PASSKEY_ACTION:
    if (e->passkey.conn_handle != connection ||
        e->passkey.params.action != BLE_SM_IOACT_DISP ||
        monotonic_ms() >= window_until) {
      ble_gap_terminate(e->passkey.conn_handle, BLE_ERR_AUTH_FAIL);
      break;
    }
    {
      uint32_t passkey;
      do {
        passkey = esp_random();
      } while (passkey >= 4294000000UL);
      passkey %= 1000000;
      if (!post({Kind::Passkey, passkey, e->passkey.conn_handle}))
        ble_gap_terminate(e->passkey.conn_handle, BLE_ERR_AUTH_FAIL);
    }
    break;
  case BLE_GAP_EVENT_SUBSCRIBE:
    if (e->subscribe.conn_handle != connection)
      break;
    if (e->subscribe.attr_handle == output_handle) {
      subscribed =
          secure(e->subscribe.conn_handle) && e->subscribe.cur_indicate;
      if (!subscribed)
        incoming.reset();
    }
    break;
  case BLE_GAP_EVENT_NOTIFY_TX:
    if (e->notify_tx.indication && e->notify_tx.status != 0 &&
        e->notify_tx.conn_handle == connection &&
        e->notify_tx.attr_handle == output_handle) {
      indication_status = e->notify_tx.status;
      xSemaphoreGive(confirmation);
    }
    break;
  default:
    break;
  }
  return 0;
}
static void host(void *) {
  nimble_port_run();
  nimble_port_freertos_deinit();
}
static void transmit(void *) {
  Tx tx = {};
  uint32_t transfer = 0;
  for (;;) {
    if (xQueueReceive(outgoing, &tx, portMAX_DELAY) != pdTRUE)
      continue;
    uint16_t conn = connection.load();
    uint32_t session = generation.load();
    if ((tx.session && tx.session != session) || !subscribed || !secure(conn)) {
      wipe(&tx, sizeof(tx));
      continue;
    }
    size_t mtu = ble_att_mtu(conn);
    if (mtu < 23)
      mtu = 23;
    size_t chunk = mtu - 11;
    if (chunk > 506)
      chunk = 506;
    ++transfer;
    uint64_t start = monotonic_ms();
    for (size_t offset = 0; offset < tx.length;) {
      if (generation != session || !subscribed || !secure(conn) ||
          monotonic_ms() - start >= 10000)
        break;
      size_t n = tx.length - offset;
      if (n > chunk)
        n = chunk;
      uint8_t packet[514];
      for (int i = 0; i < 4; ++i)
        packet[i] = transfer >> (8 * i);
      packet[4] = offset;
      packet[5] = offset >> 8;
      packet[6] = tx.length;
      packet[7] = tx.length >> 8;
      memcpy(packet + 8, tx.bytes + offset, n);
      while (xSemaphoreTake(confirmation, 0) == pdTRUE) {
      }
      indication_status = BLE_HS_EUNKNOWN;
      auto om = ble_hs_mbuf_from_flat(packet, n + 8);
      wipe(packet, sizeof(packet));
      if (!om)
        break;
      if (ble_gatts_indicate_custom(conn, output_handle, om))
        break;
      if (xSemaphoreTake(confirmation, pdMS_TO_TICKS(2000)) != pdTRUE)
        break;
      if (generation != session || indication_status != BLE_HS_EDONE)
        break;
      offset += n;
    }
    wipe(&tx, sizeof(tx));
  }
}
bool ble_init() {
  if (ble_storage_preflight() != ESP_OK) {
    // Never truncate/evict four previous bonds or overwrite unknown formats.
    // BLE stays stopped; the existing status contract exposes that state.
    ESP_LOGE(
        "sentinel",
        "BLE store incompatible; data preserved; see sketch README recovery");
    return false;
  }
  outgoing = xQueueCreate(12, sizeof(Tx));
  confirmation = xSemaphoreCreateBinary();
  if (!outgoing || !confirmation || nimble_port_init() != ESP_OK)
    return false;
  ble_hs_cfg.sync_cb = sync;
  ble_hs_cfg.sm_io_cap = BLE_HS_IO_DISPLAY_ONLY;
  ble_hs_cfg.sm_bonding = 1;
  ble_hs_cfg.sm_mitm = 1;
  ble_hs_cfg.sm_sc = 1;
  ble_hs_cfg.sm_sc_only = 1;
  ble_hs_cfg.sm_sec_lvl = 4;
  ble_hs_cfg.sm_our_key_dist =
      BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
  ble_hs_cfg.sm_their_key_dist =
      BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
  ble_hs_cfg.store_status_cb = store_status;
  ble_store_config_init();
  ble_hs_cfg.store_write_cb = secure_store_write;
  ble_svc_gap_init();
  ble_svc_gatt_init();
  ble_svc_gap_device_name_set("Sentinel");
  chars[0].uuid = &meta_uuid.u;
  chars[0].access_cb = access;
  chars[0].arg = reinterpret_cast<void *>(1);
  chars[0].flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC |
                   BLE_GATT_CHR_F_READ_AUTHEN;
  chars[0].min_key_size = 16;
  chars[1].uuid = &input_uuid.u;
  chars[1].access_cb = access;
  chars[1].arg = reinterpret_cast<void *>(2);
  chars[1].flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC |
                   BLE_GATT_CHR_F_WRITE_AUTHEN;
  chars[1].min_key_size = 16;
  chars[2].uuid = &output_uuid.u;
  chars[2].access_cb = access;
  chars[2].flags = BLE_GATT_CHR_F_INDICATE |
                   BLE_GATT_CHR_F_NOTIFY_INDICATE_ENC |
                   BLE_GATT_CHR_F_NOTIFY_INDICATE_AUTHEN;
  chars[2].val_handle = &output_handle;
  chars[2].min_key_size = 16;
  services[0].type = BLE_GATT_SVC_TYPE_PRIMARY;
  services[0].uuid = &service.u;
  services[0].characteristics = chars;
  if (ble_gatts_count_cfg(services) || ble_gatts_add_svcs(services))
    return false;
  if (xTaskCreate(transmit, "ble_tx", 6144, nullptr, 4, nullptr) != pdPASS)
    return false;
  nimble_port_freertos_init(host);
  return true;
}
bool ble_send(const char *json, size_t n, uint32_t session) {
  if (!outgoing || !subscribed || !n || n > MaxMessage)
    return false;
  Tx tx = {};
  memcpy(tx.bytes, json, n);
  tx.length = n;
  tx.session = session ? session : generation.load();
  bool ok = xQueueSend(outgoing, &tx, 0) == pdTRUE;
  wipe(&tx, sizeof(tx));
  return ok;
}
void ble_window(uint64_t deadline) { window_until = deadline; }
uint64_t ble_deadline() { return window_until.load(); }
uint32_t ble_session() { return generation.load(); }
void ble_passkey(uint16_t conn, uint32_t value, bool displayed) {
  if (!displayed || monotonic_ms() >= window_until || connection != conn) {
    ble_gap_terminate(conn, BLE_ERR_AUTH_FAIL);
    return;
  }
  ble_sm_io io = {};
  io.action = BLE_SM_IOACT_DISP;
  io.passkey = value;
  if (ble_sm_inject_io(conn, &io))
    ble_gap_terminate(conn, BLE_ERR_AUTH_FAIL);
  wipe(&io, sizeof(io));
}
const char *ble_status() {
  return connection != BLE_HS_CONN_HANDLE_NONE ? "connected"
         : host_ready                          ? "advertising"
                                               : "stopped";
}
} // namespace sentinel
