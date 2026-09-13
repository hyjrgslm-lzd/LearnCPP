#include <nghttp2/nghttp2.h>
#include <c11/socket.hpp>
#include <check.hpp>
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
using namespace std::chrono_literals;
nghttp2_nv field(std::string_view name,std::string_view value) {
    return {reinterpret_cast<std::uint8_t*>(const_cast<char*>(name.data())),reinterpret_cast<std::uint8_t*>(const_cast<char*>(value.data())),name.size(),value.size(),NGHTTP2_NV_FLAG_NONE};
}
struct endpoint {
    struct payload {std::string bytes;std::size_t offset=0;};
    nghttp2_session* session=nullptr;bool server;
    std::map<std::int32_t,payload> sending;
    std::map<std::int32_t,std::string> paths,received,course_header;
    std::map<std::int32_t,std::size_t> consumed,headers_size;
    std::map<std::int32_t,std::uint32_t> closed;
    std::uint32_t goaway=NGHTTP2_NO_ERROR;
    bool callback_failed=false;
    static nghttp2_ssize data(nghttp2_session*,std::int32_t,std::uint8_t* out,std::size_t capacity,std::uint32_t* flags,nghttp2_data_source* source,void*) {
        auto& p=*static_cast<payload*>(source->ptr);const auto count=std::min(capacity,p.bytes.size()-p.offset);
        std::memcpy(out,p.bytes.data()+p.offset,count);p.offset+=count;
        if(p.offset==p.bytes.size())*flags|=NGHTTP2_DATA_FLAG_EOF;
        return static_cast<nghttp2_ssize>(count);
    }
    static int header(nghttp2_session*,const nghttp2_frame* frame,const std::uint8_t* name,std::size_t name_size,const std::uint8_t* value,std::size_t value_size,std::uint8_t,void* user) {
        auto& self=*static_cast<endpoint*>(user);
        try {
            if(name_size>64 || value_size>512)return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            const std::string_view key(reinterpret_cast<const char*>(name),name_size),text(reinterpret_cast<const char*>(value),value_size);
            if(key==":path")self.paths[frame->hd.stream_id]=text;
            if(key=="x-course")self.course_header[frame->hd.stream_id]=text;
            return 0;
        }catch(...){self.callback_failed=true;return NGHTTP2_ERR_CALLBACK_FAILURE;}
    }
    static int frame(nghttp2_session* session,const nghttp2_frame* frame,void* user) {
        auto& self=*static_cast<endpoint*>(user);
        try {
            if(frame->hd.type==NGHTTP2_GOAWAY)self.goaway=frame->goaway.error_code;
            if(self.server && frame->hd.type==NGHTTP2_HEADERS && frame->headers.cat==NGHTTP2_HCAT_REQUEST) {
                const auto id=frame->hd.stream_id;
                auto& p=self.sending[id];p.bytes=self.paths[id]=="/small"?"ok":std::string(8192,'L');
                const std::array headers{field(":status","200"),field("x-course","learncpp")};
                nghttp2_data_provider2 provider{};provider.source.ptr=&p;provider.read_callback=data;
                return nghttp2_submit_response2(session,id,headers.data(),headers.size(),&provider);
            }
            return 0;
        }catch(...){self.callback_failed=true;return NGHTTP2_ERR_CALLBACK_FAILURE;}
    }
    static int chunk(nghttp2_session*,std::uint8_t,std::int32_t id,const std::uint8_t* data,std::size_t size,void* user) {
        auto& self=*static_cast<endpoint*>(user);
        try {
            auto& text=self.received[id];if(size>16384-text.size())return NGHTTP2_ERR_CALLBACK_FAILURE;
            text.append(reinterpret_cast<const char*>(data),size);return 0;
        }catch(...){self.callback_failed=true;return NGHTTP2_ERR_CALLBACK_FAILURE;}
    }
    static int close(nghttp2_session* session,std::int32_t id,std::uint32_t error,void* user) {
        auto& self=*static_cast<endpoint*>(user);
        try {
            self.closed[id]=error;self.sending.erase(id);
            if(!self.server) {
                const auto unconsumed=self.received[id].size()-self.consumed[id];
                if(unconsumed && nghttp2_session_consume_connection(session,unconsumed)!=0)return NGHTTP2_ERR_CALLBACK_FAILURE;
                self.consumed[id]=self.received[id].size();
            }
            return 0;
        }catch(...){self.callback_failed=true;return NGHTTP2_ERR_CALLBACK_FAILURE;}
    }
    static int sent(nghttp2_session*,const nghttp2_frame* frame,void* user) {
        auto& self=*static_cast<endpoint*>(user);
        try{if(frame->hd.type==NGHTTP2_HEADERS)self.headers_size[frame->hd.stream_id]=frame->hd.length;return 0;}
        catch(...){self.callback_failed=true;return NGHTTP2_ERR_CALLBACK_FAILURE;}
    }
    explicit endpoint(bool is_server):server(is_server) {
        nghttp2_session_callbacks* raw=nullptr;check(nghttp2_session_callbacks_new(&raw)==0,"HTTP2 callbacks allocated");
        std::unique_ptr<nghttp2_session_callbacks,decltype(&nghttp2_session_callbacks_del)> callbacks(raw,nghttp2_session_callbacks_del);
        nghttp2_session_callbacks_set_on_header_callback(raw,header);
        nghttp2_session_callbacks_set_on_frame_recv_callback(raw,frame);
        nghttp2_session_callbacks_set_on_data_chunk_recv_callback(raw,chunk);
        nghttp2_session_callbacks_set_on_stream_close_callback(raw,close);
        nghttp2_session_callbacks_set_on_frame_send_callback(raw,sent);
        nghttp2_option* option=nullptr;check(nghttp2_option_new(&option)==0,"HTTP2 options allocated");
        std::unique_ptr<nghttp2_option,decltype(&nghttp2_option_del)> options(option,nghttp2_option_del);
        if(!server)nghttp2_option_set_no_auto_window_update(option,1);
        const int status=server?nghttp2_session_server_new2(&session,raw,this,option):nghttp2_session_client_new2(&session,raw,this,option);
        check(status==0,"HTTP2 protocol session created");
        const std::array<nghttp2_settings_entry,3> settings{{{NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE,1024},{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS,16},{NGHTTP2_SETTINGS_MAX_HEADER_LIST_SIZE,8192}}};
        check(nghttp2_submit_settings(session,NGHTTP2_FLAG_NONE,settings.data(),settings.size())==0,"SETTINGS submitted");
    }
    ~endpoint(){nghttp2_session_del(session);}
    endpoint(const endpoint&)=delete;
    std::int32_t request(std::string_view path) {
        const std::array headers{field(":method","GET"),field(":scheme","http"),field(":authority","localhost"),field(":path",path),field("x-course","learncpp")};
        const auto id=nghttp2_submit_request2(session,nullptr,headers.data(),headers.size(),nullptr,nullptr);
        check(id>0,"HTTP2 stream opened");return id;
    }
};
int main() {
    c11::socket_runtime runtime;auto listener=c11::listen_loopback();check(listener.has_value(),"HTTP2 TCP listener");
    auto end=c11::clock::now()+10s;auto client_socket=c11::connect_loopback(listener->port,end);check(client_socket.has_value(),"HTTP2 TCP client");
    check(c11::wait_ready(listener->socket.get(),POLLRDNORM,end).has_value(),"HTTP2 accept ready");auto server_socket=c11::accept_socket(listener->socket.get());check(server_socket.has_value(),"HTTP2 accepted TCP socket");
    endpoint client(false),server(true);
    const auto transfer=[&](endpoint& from,endpoint& to,c11::socket_handle writer,c11::socket_handle reader){
        bool progress=false;
        for(unsigned operations=0;operations<128;++operations) {
            const std::uint8_t* bytes=nullptr;const auto count=nghttp2_session_mem_send2(from.session,&bytes);
            check(count>=0,"HTTP2 serializes pending frames");if(!count)break;
            check(c11::send_all(writer,std::span{reinterpret_cast<const char*>(bytes),static_cast<std::size_t>(count)},end).has_value(),"HTTP2 frame bytes traverse real TCP");
            auto wire=c11::receive_exact(reader,static_cast<std::size_t>(count),end);check(wire.has_value(),"HTTP2 TCP bytes collected");
            for(std::size_t at=0;at<wire->size();) {
                const auto chunk=std::min<std::size_t>(7,wire->size()-at);
                const auto n=nghttp2_session_mem_recv2(to.session,reinterpret_cast<const std::uint8_t*>(wire->data()+at),chunk);
                check(n==static_cast<nghttp2_ssize>(chunk),"HTTP2 parser consumes arbitrary TCP fragments");at+=chunk;
            }
            progress=true;
        }
        return progress;
    };
    const auto drive=[&]{for(unsigned round=0;round<128;++round){
        const bool a=transfer(client,server,client_socket->get(),server_socket->get());
        const bool b=transfer(server,client,server_socket->get(),client_socket->get());
        if(!a&&!b)return;
    }check(false,"bounded HTTP2 drive converges");};
    drive();const auto large=client.request("/large"),small=client.request("/small");drive();
    check(client.received[large].size()==1024 && !client.closed.contains(large),"large stream blocks on its advertised window");
    check(client.received[small]=="ok" && client.closed[small]==NGHTTP2_NO_ERROR,"other stream completes despite first-stream flow-control block");
    check(server.course_header[large]=="learncpp" && server.course_header[small]=="learncpp","HPACK-decoded fields retain values across streams");
    check(client.headers_size[small]<client.headers_size[large],"pinned HPACK implementation reuses dynamic table entries");
    while(client.received[large].size()<8192) {
        const auto consumed=client.received[large].size()-client.consumed[large];
        check(consumed>0 && nghttp2_session_consume(client.session,large,consumed)==0,"application consumption returns stream and connection credit");
        client.consumed[large]+=consumed;drive();
    }
    check(client.received[large]==std::string(8192,'L') && client.closed[large]==NGHTTP2_NO_ERROR,"flow-controlled stream eventually completes exact body");
    const auto cancel=client.request("/cancel");drive();check(client.received[cancel].size()==1024,"cancellation occurs with unfinished response");
    check(nghttp2_submit_rst_stream(client.session,NGHTTP2_FLAG_NONE,cancel,NGHTTP2_CANCEL)==0,"RST_STREAM submitted");drive();
    check(server.closed[cancel]==NGHTTP2_CANCEL,"peer observes stream-local cancellation");
    const auto next=client.request("/small");drive();check(client.received[next]=="ok","connection remains usable after stream reset");
    const std::array<std::uint8_t,13> invalid_window{0,0,4,NGHTTP2_WINDOW_UPDATE,0,0,0,0,0,0,0,0,0};
    check(nghttp2_session_mem_recv2(server.session,invalid_window.data(),invalid_window.size())>=0,"malformed control frame processed as protocol event");
    drive();check(client.goaway==NGHTTP2_PROTOCOL_ERROR,"zero connection window increment produces protocol GOAWAY");
    check(!client.callback_failed && !server.callback_failed,"C callbacks never leaked C++ exceptions");
    std::cout<<"real TCP HTTP2, HPACK, dual-stream flow control, reset and GOAWAY passed\n";
}
