#pragma once
#include "data_store.hpp"
#include "rf_input.hpp"
#include "dsp/engine.hpp"
#include <atomic>
#include <memory>
#include <string>

struct mg_connection;
class CivetServer;

namespace hf {

class ApiHandler;
class SseHandler;
class BandHandler;
class ModeHandler;
class StatusHandler;
class AudioHandler;

class WebServer {
public:
  WebServer(DataStore &db, RfInput &rf, DecodeEngine &engine,
            std::atomic<std::time_t> &last_capture,
            std::atomic<std::time_t> &last_decode,
            std::atomic<size_t> &last_count, const std::string &doc_root,
            int port = 8080);
  ~WebServer();

private:
  std::unique_ptr<CivetServer> server_;
  std::unique_ptr<ApiHandler> api_handler_;
  std::unique_ptr<SseHandler> sse_handler_;
  std::unique_ptr<BandHandler> band_handler_;
  std::unique_ptr<ModeHandler> mode_handler_;
  std::unique_ptr<StatusHandler> status_handler_;
  std::unique_ptr<AudioHandler> audio_handler_;
};

} // namespace hf
