#ifndef CERT_TRANS_SERVER_HANDLER_H_
#define CERT_TRANS_SERVER_HANDLER_H_

#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>

#include "util/libevent_wrapper.h"
#include "util/sync_task.h"
#include "util/task.h"

class Frontend;
template <class T>
class LogLookup;
template <class T>
class ReadOnlyDatabase;

namespace cert_trans {

class CertChain;
class CertChecker;
class SignedData;
template <class T>
class ClusterStateController;
class JsonOutput;
class LoggedCertificate;
class PreCertChain;
class Proxy;
class ThreadPool;


class HttpHandler {
 public:
  // Does not take ownership of its parameters, which must outlive
  // this instance. The "frontend" parameter can be NULL, in which
  // case this server will not accept "add-chain" and "add-pre-chain"
  // requests.
  HttpHandler(JsonOutput* json_output,
              LogLookup<LoggedCertificate>* log_lookup,
              const ReadOnlyDatabase<LoggedCertificate>* db,
              const ClusterStateController<LoggedCertificate>* controller,
              const CertChecker* cert_checker, Frontend* frontend,
              Proxy* proxy, ThreadPool* pool, libevent::Base* event_base);
  ~HttpHandler();

  void Add(libevent::HttpServer* server);

 private:
  void ProxyInterceptor(
      const libevent::HttpServer::HandlerCallback& next_handler,
      evhttp_request* request);

  void AddProxyWrappedHandler(
      libevent::HttpServer* server, const std::string& path,
      const libevent::HttpServer::HandlerCallback& local_handler);

  void GetEntries(evhttp_request* req) const;
  void GetRoots(evhttp_request* req) const;
  void GetProof(evhttp_request* req) const;
  void GetSTH(evhttp_request* req) const;
  void GetConsistency(evhttp_request* req) const;
  void AddSignedData(evhttp_request* req);
  void AddChain(evhttp_request* req);
  void AddPreChain(evhttp_request* req);

  void BlockingGetEntries(evhttp_request* req, int64_t start, int64_t end,
                          bool include_scts) const;
  void BlockingAddChain(evhttp_request* req,
                        const std::shared_ptr<CertChain>& chain) const;
  void BlockingAddSignedData(evhttp_request* req,
                        const std::shared_ptr<SignedData>& data) const;
  void BlockingAddPreChain(evhttp_request* req,
                           const std::shared_ptr<PreCertChain>& chain) const;

  bool IsNodeStale() const;
  void UpdateNodeStaleness();

  JsonOutput* const output_;
  LogLookup<LoggedCertificate>* const log_lookup_;
  const ReadOnlyDatabase<LoggedCertificate>* const db_;
  const ClusterStateController<LoggedCertificate>* const controller_;
  const CertChecker* const cert_checker_;
  Frontend* const frontend_;
  Proxy* const proxy_;
  ThreadPool* const pool_;
  libevent::Base* const event_base_;

  util::SyncTask task_;
  mutable std::mutex mutex_;
  bool node_is_stale_;

  DISALLOW_COPY_AND_ASSIGN(HttpHandler);
};


}  // namespace cert_trans

#endif  // CERT_TRANS_SERVER_HANDLER_H_
