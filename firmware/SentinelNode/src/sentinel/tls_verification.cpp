#include "include/tls_verification.h"
#include "include/transport_requirements.h"
#include <memory>
#include <new>
namespace sentinel {
bool verify_peer_dates(NetworkClientSecure &tls, const char *ca, time_t now,
                       bool time_valid) {
  if (!time_valid || !ca || !*ca)
    return false;
  const auto *peer = tls.getPeerCertificate();
  if (!peer)
    return false;
  bssl::X509List anchors(ca);
  if (!anchors.getCount())
    return false;
  auto ctx = std::unique_ptr<br_x509_minimal_context>(
      new (std::nothrow) br_x509_minimal_context);
  if (!ctx)
    return false;
  int64_t days = static_cast<int64_t>(now) / 86400;
  int64_t seconds = static_cast<int64_t>(now) % 86400;
  if (seconds < 0) {
    --days;
    seconds += 86400;
  }
  days += 719528;
  if (days < 0 || days > UINT32_MAX)
    return false;
  const auto **v = &ctx->vtable;
  auto begin = [&](size_t root) {
    br_x509_minimal_init_full(ctx.get(), anchors.getTrustAnchors() + root, 1);
    br_x509_minimal_set_time(ctx.get(), days, seconds);
    (*v)->start_chain(v, nullptr);
  };
  auto append = [&](const unsigned char *data, size_t size) {
    (*v)->start_cert(v, size);
    (*v)->append(v, data, size);
    (*v)->end_cert(v);
  };
  // Trust-anchor structures omit validity dates. Validate each configured CA
  // certificate itself before using that single anchor for the complete peer
  // chain. An expired unused CA in a bundle cannot disable a valid path.
  const auto *certificates = anchors.getX509Certs();
  for (size_t root = 0; root < anchors.getCount(); ++root) {
    begin(root);
    append(certificates[root].data, certificates[root].data_len);
    if ((*v)->end_chain(v) != 0) continue;
    begin(root);
    // Native TLS already required the original DNS/IP identity. The maintained
    // second engine supplies certificate dates for the same CA and peer DER.
    for (auto crt = peer; crt; crt = crt->next) {
      if (!crt->raw.p || !crt->raw.len) return false;
      append(crt->raw.p, crt->raw.len);
    }
    if ((*v)->end_chain(v) == 0) return true;
  }
  return false;
}
} // namespace sentinel
