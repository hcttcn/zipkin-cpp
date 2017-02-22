#pragma once

#include <arpa/inet.h>

#include <cstdint>
#include <memory>
#include <chrono>

#include <thrift/protocol/TProtocol.h>

#include "../gen-cpp/zipkinCore_constants.h"

typedef uint64_t span_id_t;
typedef uint64_t trace_id_t;
typedef uint64_t timestamp_t;
typedef void *userdata_t;

namespace zipkin
{

struct Endpoint
{
    sockaddr_in addr;
    const std::string service;

    const ::Endpoint host(void) const;
};

struct Tracer;

class CachedTracer;

struct Span
{
  protected:
    Tracer *m_tracer;
    ::Span m_span;
    userdata_t m_userdata;

    static const ::Endpoint host(const Endpoint *endpoint);

  public:
    Span(Tracer *tracer, const std::string &name, span_id_t parent_id = 0, userdata_t userdata = nullptr);

    void reset(const std::string &name, span_id_t parent_id = 0, userdata_t userdata = nullptr);

    void submit(void);

    Tracer *tracer(void) const { return m_tracer; }
    const ::Span &message(void) const { return m_span; }

    trace_id_t trace_id(void) const { return m_span.trace_id; }

    span_id_t id(void) const { return m_span.id; }
    void set_id(span_id_t id) { m_span.id = id; }

    const std::string &name(void) const { return m_span.name; }

    span_id_t parent_id(void) const { return m_span.parent_id; }
    void set_parent_id(span_id_t id) { m_span.parent_id = id; }

    userdata_t userdata(void) const { return m_userdata; }
    void set_userdata(userdata_t userdata) { m_userdata = userdata; }

    virtual Span *span(const std::string &name, userdata_t userdata = nullptr) const
    {
        return new Span(m_tracer, name, m_span.id, userdata ? userdata : m_userdata);
    };

    static uint64_t next_id();

    static timestamp_t now();

    void annotate(const std::string &value, const Endpoint *endpoint = nullptr);

    inline void client_send(const Endpoint *endpoint = nullptr) { annotate(g_zipkinCore_constants.CLIENT_SEND, endpoint); }
    inline void client_recv(const Endpoint *endpoint = nullptr) { annotate(g_zipkinCore_constants.CLIENT_RECV, endpoint); }
    inline void server_send(const Endpoint *endpoint = nullptr) { annotate(g_zipkinCore_constants.SERVER_SEND, endpoint); }
    inline void server_recv(const Endpoint *endpoint = nullptr) { annotate(g_zipkinCore_constants.SERVER_RECV, endpoint); }
    inline void wire_send(const Endpoint *endpoint = nullptr) { annotate(g_zipkinCore_constants.WIRE_SEND, endpoint); }
    inline void wire_recv(const Endpoint *endpoint = nullptr) { annotate(g_zipkinCore_constants.WIRE_RECV, endpoint); }
    inline void client_send_fragment(const Endpoint *endpoint = nullptr) { annotate(g_zipkinCore_constants.CLIENT_SEND_FRAGMENT, endpoint); }
    inline void client_recv_fragment(const Endpoint *endpoint = nullptr) { annotate(g_zipkinCore_constants.CLIENT_RECV_FRAGMENT, endpoint); }
    inline void server_send_fragment(const Endpoint *endpoint = nullptr) { annotate(g_zipkinCore_constants.SERVER_SEND_FRAGMENT, endpoint); }
    inline void server_recv_fragment(const Endpoint *endpoint = nullptr) { annotate(g_zipkinCore_constants.SERVER_RECV_FRAGMENT, endpoint); }

    template <typename T>
    void annotate(const std::string &key, T value, const Endpoint *endpoint = nullptr);

    void annotate(const std::string &key, const uint8_t *value, size_t size, const Endpoint *endpoint = nullptr);

    template <size_t N>
    inline void annotate(const std::string &key, const uint8_t (&value)[N], const Endpoint *endpoint = nullptr)
    {
        annotate(key, value, N, endpoint);
    }
    inline void annotate(const std::string &key, const std::vector<uint8_t> &value, const Endpoint *endpoint = nullptr)
    {
        annotate(key, value.data(), value.size(), endpoint);
    }
    void annotate(const std::string &key, const std::string &value, const Endpoint *endpoint = nullptr);
    inline void annotate(const std::string &key, const char *value, int len = -1, const Endpoint *endpoint = nullptr)
    {
        annotate(key, len >= 0 ? std::string(value, len) : std::string(value), endpoint);
    }
    void annotate(const std::string &key, const std::wstring &value, const Endpoint *endpoint = nullptr);
    inline void annotate(const std::string &key, const wchar_t *value, int len = -1, const Endpoint *endpoint = nullptr)
    {
        annotate(key, len >= 0 ? std::wstring(value, len) : std::wstring(value), endpoint);
    }

    inline void http_host(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_HOST, value, endpoint);
    }
    inline void http_method(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_METHOD, value, endpoint);
    }
    inline void http_path(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_PATH, value, endpoint);
    }
    inline void http_url(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_URL, value, endpoint);
    }
    inline void http_status_code(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_STATUS_CODE, value, endpoint);
    }
    inline void http_request_size(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_REQUEST_SIZE, value, endpoint);
    }
    inline void http_response_size(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_RESPONSE_SIZE, value, endpoint);
    }
    inline void local_component(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.LOCAL_COMPONENT, value, endpoint);
    }
    inline void client_addr(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.CLIENT_ADDR, value, endpoint);
    }
    inline void server_addr(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.SERVER_ADDR, value, endpoint);
    }

    size_t serialize_binary(apache::thrift::protocol::TProtocol &protocol) const
    {
        return m_span.write(&protocol);
    }

    template <class Writer>
    void serialize_json(Writer &writer) const;
};

class CachedSpan : public Span
{
    uint8_t m_buf[0] __attribute__((aligned));

  public:
    CachedSpan(Tracer *tracer, const std::string &name, span_id_t parent_id = 0, userdata_t userdata = nullptr)
        : Span(tracer, name, parent_id, userdata)
    {
    }

    static void *operator new(size_t size, CachedTracer *tracer) noexcept;
    static void operator delete(void *ptr, std::size_t sz) noexcept;

    static size_t cache_offset(void) { return offsetof(CachedSpan, m_buf); }

    uint8_t *cache_ptr(void) { return &m_buf[0]; }
    size_t cache_size(void) const;

    void release(void);

    virtual Span *span(const std::string &name, userdata_t userdata = nullptr) const override;

} __attribute__((aligned));

namespace __impl
{
template <typename T>
struct __annotation
{
};
template <>
struct __annotation<bool>
{
    static const AnnotationType::type type = AnnotationType::type::BOOL;
};
template <>
struct __annotation<int16_t>
{
    static const AnnotationType::type type = AnnotationType::type::I16;
};
template <>
struct __annotation<int32_t>
{
    static const AnnotationType::type type = AnnotationType::type::I32;
};
template <>
struct __annotation<int64_t>
{
    static const AnnotationType::type type = AnnotationType::type::I64;
};
template <>
struct __annotation<double>
{
    static const AnnotationType::type type = AnnotationType::type::DOUBLE;
};
} // namespace __impl

template <typename T>
inline void Span::annotate(const std::string &key, T value, const Endpoint *endpoint)
{
    auto ptr = reinterpret_cast<uint8_t *>(&value);
    std::string data(reinterpret_cast<char *>(ptr), sizeof(T));

    ::BinaryAnnotation annotation;

    annotation.__set_key(key);
    annotation.__set_value(data);
    annotation.__set_annotation_type(__impl::__annotation<T>::type);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_span.binary_annotations.push_back(annotation);
}

template <class Writer>
void Span::serialize_json(Writer &writer) const
{
    auto serialize_endpoint = [&writer](const ::Endpoint &host) {
        writer.StartObject();

        writer.Key("serviceName");
        writer.String(host.service_name);

        writer.Key("ipv4");
        writer.String(inet_ntoa({static_cast<in_addr_t>(host.ipv4)}));

        writer.Key("port");
        writer.Int(host.port);

        writer.EndObject();
    };

    auto serialize_value = [&writer](const std::string &data, AnnotationType::type type) {
        switch (type)
        {
        case AnnotationType::type::BOOL:
            writer.Bool(*reinterpret_cast<const bool *>(data.c_str()));
            break;

        case AnnotationType::type::I16:
            writer.Int(*reinterpret_cast<const int16_t *>(data.c_str()));
            break;

        case AnnotationType::type::I32:
            writer.Int(*reinterpret_cast<const int32_t *>(data.c_str()));
            break;

        case AnnotationType::type::I64:
            writer.Int64(*reinterpret_cast<const int64_t *>(data.c_str()));
            break;

        case AnnotationType::type::DOUBLE:
            writer.Double(*reinterpret_cast<const double *>(data.c_str()));
            break;

        case AnnotationType::type::BYTES:
            writer.StartArray();

            for (auto c : data)
            {
                writer.Int(c);
            }

            writer.EndArray(data.size());

            break;

        case AnnotationType::type::STRING:
            writer.String(data);
            break;
        }
    };

    char str[64];

    writer.StartObject();

    writer.Key("traceId");
    writer.String(str, snprintf(str, sizeof(str), "%016llx", m_span.trace_id));

    writer.Key("name");
    writer.String(m_span.name);

    writer.Key("id");
    writer.String(str, snprintf(str, sizeof(str), "%016llx", m_span.id));

    if (m_span.__isset.parent_id)
    {
        writer.Key("parentId");
        writer.String(str, snprintf(str, sizeof(str), "%016llx", m_span.parent_id));
    }

    writer.Key("annotations");
    writer.StartArray();

    for (auto &annotation : m_span.annotations)
    {
        writer.StartObject();

        if (annotation.__isset.host)
        {
            writer.Key("endpoint");
            serialize_endpoint(annotation.host);
        }

        writer.Key("timestamp");
        writer.Int64(annotation.timestamp);

        writer.Key("value");
        writer.String(annotation.value);

        writer.EndObject();
    }

    writer.EndArray(m_span.annotations.size());

    writer.Key("binary_annotations");
    writer.StartArray();

    for (auto &annotation : m_span.binary_annotations)
    {
        writer.StartObject();

        if (annotation.__isset.host)
        {
            writer.Key("endpoint");
            serialize_endpoint(annotation.host);
        }

        writer.Key("key");
        writer.String(annotation.key);

        writer.Key("value");
        serialize_value(annotation.value, annotation.annotation_type);

        writer.EndObject();
    }

    writer.EndArray(m_span.binary_annotations.size());

    if (m_span.__isset.debug)
    {
        writer.Key("debug");
        writer.Bool(m_span.debug);
    }

    if (m_span.__isset.timestamp)
    {
        writer.Key("timestamp");
        writer.Int64(m_span.timestamp);
    }

    if (m_span.__isset.duration)
    {
        writer.Key("duration");
        writer.Int64(m_span.duration);
    }

    writer.EndObject();
}

} // namespace zipkin