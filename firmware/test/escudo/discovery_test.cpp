#include "test_support.hpp"
static void catalog_tests() {
  Catalog catalog;
  catalog.reset("scan-A");
  auto n = network(1, "");
  catalog.add(n, 10);
  catalog.add(network(2, "SHARED"), 20);
  catalog.add(network(3, "SHARED"), 20);
  n.rssi = -30;
  catalog.add(n, 30);
  assert(catalog.size() == 3);
  cJSON after = {};
  after.type = cJSON_NULL;
  cJSON *page = nullptr;
  assert(!catalog.page(&after, 4, page));
  assert(v2::validate("V2NetworksPage", page));
  auto rows = cJSON_GetObjectItemCaseSensitive(page, "networks");
  assert(cJSON_GetArraySize(rows) == 3);
  auto hidden = cJSON_GetArrayItem(rows, 0);
  assert(!strcmp(text(hidden, "ssid"), "") &&
         number(hidden, "seen_uptime_ms") == 30);
  assert(cJSON_GetObjectItemCaseSensitive(hidden, "rssi")->valueint == -30);
  assert(!strcmp(text(hidden, "pmf"), "unknown"));
  assert(!cJSON_GetObjectItemCaseSensitive(page, "malicious"));
  cJSON_Delete(page);
  for (unsigned i = 4; i <= 40; ++i)
    catalog.add(network(i), 40);
  assert(catalog.size() == 32 && catalog.dropped() == 8);
  size_t seen = 0;
  cJSON *cursor = cJSON_CreateNull();
  std::string stale;
  do {
    assert(!catalog.page(cursor, 4, page));
    cJSON_Delete(cursor);
    assert(v2::validate("V2NetworksPage", page));
    rows = cJSON_GetObjectItemCaseSensitive(page, "networks");
    assert(cJSON_GetArraySize(rows) <= 4);
    seen += cJSON_GetArraySize(rows);
    cursor =
        cJSON_Duplicate(cJSON_GetObjectItemCaseSensitive(page, "next"), true);
    if (cJSON_IsString(cursor))
      stale = cursor->valuestring;
    cJSON_Delete(page);
  } while (!cJSON_IsNull(cursor));
  cJSON_Delete(cursor);
  assert(seen == 32);
  auto forged = cJSON_CreateString("scan-A:31");
  assert(!strcmp(catalog.page(forged, 4, page), "invalid_args"));
  cJSON_Delete(forged);
  catalog.reset("scan-B");
  forged = cJSON_CreateString(stale.c_str());
  assert(!strcmp(catalog.page(forged, 4, page), "revision_conflict"));
  cJSON_Delete(forged);
  assert(!catalog.page(&after, 4, page));
  assert(cJSON_GetArraySize(
             cJSON_GetObjectItemCaseSensitive(page, "networks")) == 0 &&
         !cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(page, "more")));
  cJSON_Delete(page);
  n = network(1);
  n.ssid[0] = char(0xff);
  n.ssid[1] = char(0xc0);
  n.ssid[2] = 0;
  catalog.add(n, 40);
  assert(!catalog.page(&after, 4, page));
  assert(v2::validate("V2NetworksPage", page));
  rows = cJSON_GetObjectItemCaseSensitive(page, "networks");
  auto ssid =
      cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(rows, 0), "ssid");
  assert(!strcmp(text(ssid, "value"), "ffc0"));
  auto target = Catalog::target_json({n, 40});
  assert(catalog.find(target));
  cJSON_Delete(target);
  cJSON_Delete(page);
  std::string longest(64, 'x');
  catalog.reset(longest.c_str());
  for (unsigned i = 1; i <= 4; ++i) {
    n = network(i);
    memset(n.ssid, '"', 32);
    n.ssid[32] = 0;
    n.auth = 7;
    catalog.add(n, MaxInteger);
  }
  assert(!catalog.page(&after, 4, page));
  cJSON_AddStringToObject(page, "kind", "escudo.networks");
  Result result;
  strcpy(result.id, longest.c_str());
  assert(
      cJSON_PrintPreallocated(page, result.body, sizeof(result.body), false));
  cJSON_Delete(page);
  Envelope bound = {longest.c_str(), longest.c_str(), MaxInteger, MaxInteger};
  bound.version = 2;
  bound.timestamp = "2026-11-11T22:00:00.12345678901Z";
  bound.quality = "app_sync";
  char bounded[MaxMessage + 1];
  assert(serialize(
      envelope(bound, "ack", module_ack_payload(result, "applied")), bounded));

  assert(!strcmp(auth_label(6), "wpa3_personal") &&
         !strcmp(auth_label(5), "enterprise") &&
         !strcmp(auth_label(9), "unknown"));
  assert(!strcmp(cipher_label(4, 3), "mixed") &&
         !strcmp(cipher_label(12, 12), "unknown"));
}
static const char *target =
    "{\"ssid\":\"LAB\",\"bssid\":\"02:00:00:00:00:01\",\"channel\":6}";
static void module_tests() {
  Harness h;
  assert(h.supervisor.capabilities().capture &&
         !h.supervisor.capabilities().detectors);
  h.state();
  auto scan = h.command_text("escudo.scan", "{}", "scan-main");
  auto denied = h.send(scan, false);
  assert(denied.error && !strcmp(denied.error, "unauthorized") &&
         h.radio.scan_calls == 0);
  auto started = h.send(scan);
  assert(!started.error && strstr(started.body, "scan-main") &&
         h.radio.scan_calls == 1);
  assert(!h.send(scan).error && h.radio.scan_calls == 1);
  assert(!strcmp(h.command("escudo.scan").error, "busy"));
  assert(!strcmp(
      h.command("escudo.networks", "{\"after\":null,\"limit\":4}").error,
      "busy"));
  for (unsigned i = 1; i <= 6; ++i)
    h.radio.records.push_back(network(i));
  h.radio.done();
  h.tick();
  assert(h.context.events.size() == 1);
  auto event = cJSON_Parse(h.context.events.back().c_str());
  auto payload = cJSON_GetObjectItemCaseSensitive(event, "payload");
  assert(!strcmp(text(payload, "origin"), "simulated"));
  auto details = cJSON_GetObjectItemCaseSensitive(payload, "details");
  assert(!strcmp(text(details, "scan_id"), "scan-main"));
  cJSON_Delete(event);
  auto page = h.command("escudo.networks", "{\"after\":null,\"limit\":4}");
  assert(!page.error);
  auto j = cJSON_Parse(page.body);
  std::string cursor = text(j, "next");
  cJSON_Delete(j);
  std::string args = "{\"after\":\"" + cursor + "\",\"limit\":4}";
  page = h.command("escudo.networks", args.c_str());
  assert(!page.error);
  auto selected = h.command("escudo.select_target", target);
  assert(!selected.error && strstr(selected.body, "02:00:00:00:00:01"));
  h.tick();
  h.state();
  assert(!strcmp(
      h.command(
           "escudo.select_target",
           "{\"ssid\":\"LAB\",\"bssid\":\"02:00:00:00:ff:ff\",\"channel\":6}")
          .error,
      "invalid_args"));
  assert(!strcmp(
      h.command(
           "escudo.select_target",
           "{\"ssid\":\"LAB\",\"bssid\":\"02:00:00:00:00:01\",\"channel\":11}")
          .error,
      "invalid_args"));
  assert(!strcmp(
      h.command(
           "escudo.select_target",
           "{\"ssid\":\"OTHER\",\"bssid\":\"02:00:00:00:00:01\",\"channel\":6}")
          .error,
      "invalid_args"));
  ModuleSnapshot snapshot;
  h.module.snapshot(snapshot);
  assert(strstr(snapshot.state, "02:00:00:00:00:01"));
  assert(!h.command("escudo.capture_start").error);
  assert(!h.command("escudo.capture_stop").error);
  h.radio.channel_ok = false;
  assert(!strcmp(
      h.command(
           "escudo.select_target",
           "{\"ssid\":\"LAB\",\"bssid\":\"02:00:00:00:00:02\",\"channel\":6}")
          .error,
      "resource_error"));
  h.module.snapshot(snapshot);
  assert(strstr(snapshot.state, "02:00:00:00:00:01"));
  h.radio.channel_ok = true;
  h.context.sink_ready = false;
  assert(!h.command("escudo.select_target", target).error);
  h.tick();
  assert(!strcmp(h.command("escudo.select_target", target).error, "busy"));
  h.context.sink_ready = true;
  h.tick();
  assert(!h.command("escudo.scan").error);
  h.radio.done();
  h.tick();
  assert(!strcmp(h.command("escudo.networks", args.c_str()).error,
                 "revision_conflict"));
  assert(!h.command("escudo.scan").error);
  assert(h.radio.scanning);
  unsigned stops = h.radio.stops;
  assert(!h.supervisor.transition(Mode::Idle, h.context.now) &&
         h.module.stopped() && !h.radio.scanning && h.radio.stops > stops);
  h.state();
  assert(!h.supervisor.transition(Mode::Escudo, h.context.now));
  assert(!h.command("escudo.scan").error);
  h.radio.records.clear();
  h.radio.done();
  h.tick();
  page = h.command("escudo.networks", "{\"after\":null,\"limit\":4}");
  assert(!page.error && strstr(page.body, "\"networks\":[]"));
  assert(!h.command("escudo.scan").error);
  h.radio.done(1);
  h.tick();
  h.module.snapshot(snapshot);
  assert(strstr(snapshot.state, "\"scan_status\":\"error\""));
  assert(!h.command("escudo.scan").error);
  h.radio.read_ok = false;
  h.radio.done();
  h.tick();
  assert(!strcmp(
      h.command("escudo.networks", "{\"after\":null,\"limit\":4}").error,
      "invalid_state"));
  h.radio.read_ok = true;
  assert(!h.command("escudo.scan").error);
  h.context.now += 10000;
  h.tick();
  h.module.snapshot(snapshot);
  assert(strstr(snapshot.state, "\"scan_status\":\"error\""));
  assert(!strcmp(h.command("escudo.scan").error, "invalid_state"));
  assert(!h.supervisor.transition(Mode::Idle, h.context.now));
  assert(!h.supervisor.transition(Mode::Escudo, h.context.now));
  assert(!h.command("escudo.scan").error);
  assert(!h.supervisor.transition(Mode::Idle, h.context.now));
}
int main(int argc, char **argv) {
  emit_wire = argc == 2 && !strcmp(argv[1], "wire");
  if (!emit_wire)
    catalog_tests();
  module_tests();
  if (!emit_wire)
    std::cout << "Escudo: catalog/SSID/dedup/overflow/cursors, real module "
                 "hooks, gate/ACK, selection, scan errors, backpressure and "
                 "Idle/restart: PASS (host)\n";
}
