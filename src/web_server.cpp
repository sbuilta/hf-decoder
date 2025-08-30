#include "web_server.hpp"
#include "CivetServer.h"
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>

namespace hf {

class ApiHandler : public CivetHandler {
public:
  explicit ApiHandler(DataStore &db) : db_(db) {}
  bool handleGet(CivetServer *server, struct mg_connection *conn) override {
    auto recs = db_.recent(10);
    std::ostringstream os;
    os << "[";
    for (size_t i = 0; i < recs.size(); ++i) {
      const auto &r = recs[i];
      os << "{\"timestamp\":" << r.timestamp << ",\"band\":\"" << r.band
         << "\",\"frequency\":" << r.frequency_hz << ",\"mode\":\""
         << mode_to_string(r.mode) << "\",\"snr\":" << r.snr_db
         << ",\"text\":\"" << r.text << "\"}";
      if (i + 1 != recs.size())
        os << ",";
    }
    os << "]";
    std::string body = os.str();
    mg_printf(conn,
              "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
              "Connection: close\r\n\r\n%s",
              body.c_str());
    return true;
  }

private:
  DataStore &db_;
};

class SseHandler : public CivetHandler {
public:
  bool handleGet(CivetServer *server, struct mg_connection *conn) override {
    mg_printf(conn,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/event-stream\r\n"
              "Cache-Control: no-cache\r\n"
              "Connection: keep-alive\r\n\r\n");
    for (;;) {
      if (mg_printf(conn, "data: tick\n\n") <= 0)
        break;
      std::this_thread::sleep_for(std::chrono::seconds(15));
    }
    return true;
  }
};

class BandHandler : public CivetHandler {
public:
  explicit BandHandler(RfInput &rf) : rf_(rf) {}
  bool handleGet(CivetServer *server, struct mg_connection *conn) override {
    auto &presets = rf_.presets();
    std::ostringstream os;
    os << "{\"current\":" << rf_.current_band() << ",\"presets\":[";
    for (size_t i = 0; i < presets.size(); ++i) {
      const auto &p = presets[i];
      os << "{\"name\":\"" << p.name << "\",\"freq\":"
         << p.center_freq_hz << "}";
      if (i + 1 != presets.size())
        os << ",";
    }
    os << "]}";
    mg_printf(conn,
              "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
              "Connection: close\r\n\r\n%s",
              os.str().c_str());
    return true;
  }
  bool handlePost(CivetServer *server, struct mg_connection *conn) override {
    const struct mg_request_info *ri = mg_get_request_info(conn);
    char buf[16];
    size_t len = ri->query_string ? strlen(ri->query_string) : 0;
    if (mg_get_var(ri->query_string, len, "index", buf, sizeof(buf)) > 0) {
      size_t idx = static_cast<size_t>(strtoul(buf, nullptr, 10));
      bool ok = rf_.set_band(idx);
      mg_printf(conn,
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
                "Connection: close\r\n\r\n{\"ok\":%s}",
                ok ? "true" : "false");
      return true;
    }
    mg_printf(conn,
              "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\n"
              "Connection: close\r\n\r\n{\"error\":\"missing index\"}");
    return true;
  }

private:
  RfInput &rf_;
};

class ModeHandler : public CivetHandler {
public:
  explicit ModeHandler(DecodeEngine &e) : engine_(e) {}
  bool handleGet(CivetServer *server, struct mg_connection *conn) override {
    mg_printf(conn,
              "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
              "Connection: close\r\n\r\n{\"js8\":%s}",
              engine_.js8_enabled() ? "true" : "false");
    return true;
  }
  bool handlePost(CivetServer *server, struct mg_connection *conn) override {
    const struct mg_request_info *ri = mg_get_request_info(conn);
    char buf[8];
    size_t len = ri->query_string ? strlen(ri->query_string) : 0;
    if (mg_get_var(ri->query_string, len, "js8", buf, sizeof(buf)) > 0) {
      bool en = std::strtol(buf, nullptr, 10) != 0;
      engine_.set_js8_enabled(en);
      mg_printf(conn,
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
                "Connection: close\r\n\r\n{\"js8\":%s}",
                en ? "true" : "false");
      return true;
    }
    mg_printf(conn,
              "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\n"
              "Connection: close\r\n\r\n{\"error\":\"missing js8 param\"}");
    return true;
  }

private:
  DecodeEngine &engine_;
};

WebServer::WebServer(DataStore &db, RfInput &rf, DecodeEngine &engine,
                     const std::string &doc_root, int port)
    : server_(nullptr), api_handler_(nullptr), sse_handler_(nullptr),
      band_handler_(nullptr), mode_handler_(nullptr) {
  std::vector<std::string> opts = {"document_root", doc_root,
                                   "listening_ports", std::to_string(port)};
  server_ = std::make_unique<CivetServer>(opts);
  api_handler_ = std::make_unique<ApiHandler>(db);
  sse_handler_ = std::make_unique<SseHandler>();
  band_handler_ = std::make_unique<BandHandler>(rf);
  mode_handler_ = std::make_unique<ModeHandler>(engine);
  server_->addHandler("/api/messages", *api_handler_);
  server_->addHandler("/events", *sse_handler_);
  server_->addHandler("/api/band", *band_handler_);
  server_->addHandler("/api/mode", *mode_handler_);
}

WebServer::~WebServer() {
  if (server_) {
    server_->removeHandler("/api/messages");
    server_->removeHandler("/events");
    server_->removeHandler("/api/band");
    server_->removeHandler("/api/mode");
  }
}

} // namespace hf
