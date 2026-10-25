
#include "emu.h"
#include "http.h"

// Complete types needed for member cleanup.
namespace asio {
    class io_context {};
}

namespace webpp {
    class http_server {};
    class ws_server {};
}

// Offline HTTP manager for Batman PS4.
http_manager::http_manager(bool, short, const char *root)
    : m_active(false),
      m_root(root ? root : "")
{
}

http_manager::~http_manager() = default;

void http_manager::clear() {}

void http_manager::add_template(
    const std::string&, substitution, char, char) {}

void http_manager::remove_template(
    const std::string&) {}

void http_manager::serve_template(
    http_request_ptr, http_response_ptr,
    const std::string&, substitution, char, char) {}

void http_manager::serve_document(
    http_request_ptr, http_response_ptr,
    const std::string&) {}

void http_manager::add_http_handler(
    const std::string&, http_handler) {}

void http_manager::remove_http_handler(
    const std::string&) {}

http_manager::websocket_endpoint_ptr
http_manager::add_endpoint(
    const std::string&,
    websocket_open_handler,
    websocket_message_handler,
    websocket_close_handler,
    websocket_error_handler)
{
    return {};
}

void http_manager::remove_endpoint(
    const std::string&) {}
