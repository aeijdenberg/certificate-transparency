#include "log/logged_certificate.h"

using ct::LogEntry;
using ct::PreCert;
using ct::SignedDataEntry;
using ct::SignedCertificateTimestamp;

namespace cert_trans {


bool LoggedCertificate::CopyFromClientLogEntry(
    const AsyncLogClient::Entry& entry) {
  if (entry.leaf.timestamped_entry().entry_type() != ct::X509_ENTRY &&
      entry.leaf.timestamped_entry().entry_type() != ct::PRECERT_ENTRY &&
      entry.leaf.timestamped_entry().entry_type() != ct::SIGNED_DATA_ENTRY) {
    LOG(INFO) << "unsupported entry_type: "
              << entry.leaf.timestamped_entry().entry_type();
    return false;
  }

  Clear();

  ct::SignedCertificateTimestamp* const sct(mutable_contents()->mutable_sct());
  sct->set_version(ct::V1);
  sct->set_timestamp(entry.leaf.timestamped_entry().timestamp());
  sct->set_extensions(entry.leaf.timestamped_entry().extensions());

  // It may look like you should just be able to copy entry.entry over
  // contents.entry, but entry.entry is incomplete (when the same
  // information is available in entry.leaf, it will be missing from
  // entry.entry). So we still need to fill in some missing bits...
  LogEntry* const log_entry(mutable_contents()->mutable_entry());
  log_entry->CopyFrom(entry.entry);
  log_entry->set_type(entry.leaf.timestamped_entry().entry_type());
  switch (contents().entry().type()) {
    case ct::X509_ENTRY: {
      log_entry->mutable_x509_entry()->set_leaf_certificate(
          entry.leaf.timestamped_entry().signed_entry().x509());
      break;
    }

    case ct::PRECERT_ENTRY: {
      PreCert* const precert(
          log_entry->mutable_precert_entry()->mutable_pre_cert());
      precert->set_issuer_key_hash(entry.leaf.timestamped_entry()
                                       .signed_entry()
                                       .precert()
                                       .issuer_key_hash());
      precert->set_tbs_certificate(entry.leaf.timestamped_entry()
                                       .signed_entry()
                                       .precert()
                                       .tbs_certificate());
      break;
    }

    case ct::SIGNED_DATA_ENTRY: {
      SignedDataEntry* const signeddataentry(log_entry->mutable_signed_data_entry());
      signeddataentry->set_keyid(entry.leaf.timestamped_entry()
                                           .signed_entry()
                                           .data()
                                           .keyid());
      signeddataentry->set_data(entry.leaf.timestamped_entry()
                                           .signed_entry()
                                           .data()
                                           .data());
      break;
    }

    default:
      LOG(FATAL) << "unknown entry type";
  }

  return true;
}


}  // namespace cert_trans
