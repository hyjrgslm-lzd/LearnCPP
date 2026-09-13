#include <c11/socket.hpp>
#include <msquic.h>
#include <c11/certificate_fixture.hpp>
#include <check.hpp>
#include <condition_variable>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
using namespace std::chrono_literals;

struct experiment {
    struct connection {experiment* owner{};HQUIC handle{};bool peer=false,connected=false,done=false;QUIC_STATUS transport=QUIC_STATUS_SUCCESS;};
    struct stream {
        experiment* owner{};HQUIC handle{};std::uint64_t id=UINT64_MAX;
        bool peer=false,uni=false,started=false,done=false,send_pending=false,send_canceled=false,aborted=false,release_receive=false;
        std::uint64_t pending=0;std::string incoming,outgoing;QUIC_BUFFER buffer{};
    };
    const QUIC_API_TABLE* api=nullptr;HQUIC registration{},listener{},server_config{},client_config{};
    connection client{this},server{this,nullptr,true};
    std::array<stream,3> clients{},peers{};std::size_t peer_count=0;
    std::mutex mutex;std::condition_variable changed;bool resume_large=false,failed=false,closed=false;
    std::filesystem::path directory;std::string cert_file,key_file,ca_file;
    static constexpr char protocol[]="c11-lab";
    QUIC_BUFFER alpn{sizeof(protocol)-1,reinterpret_cast<std::uint8_t*>(const_cast<char*>(protocol))};
    static void good(QUIC_STATUS status,const char* message){if(QUIC_FAILED(status))std::cerr<<message<<" status=0x"<<std::hex<<static_cast<std::uint32_t>(status)<<std::dec<<'\n';check(QUIC_SUCCEEDED(status),message);}
    template<class F> void wait(F ready) {
        std::unique_lock lock(mutex);
        const bool arrived=changed.wait_for(lock,10s,[&]{return failed || ready();});
        if(!arrived || failed)std::cerr<<"QUIC state client_connected="<<client.connected<<" client_done="<<client.done<<" server_connected="<<server.connected<<" peers="<<peer_count<<" transport=0x"<<std::hex<<static_cast<std::uint32_t>(client.transport)<<std::dec<<'\n';
        check(arrived && !failed,"QUIC expected event arrives within bounded wait");
    }
    void send(stream& state,std::string_view bytes) {
        std::unique_lock lock(mutex);
        if(!state.handle || state.send_pending || bytes.size()>65536){failed=true;changed.notify_all();return;}
        state.outgoing=bytes;state.buffer={static_cast<std::uint32_t>(state.outgoing.size()),reinterpret_cast<std::uint8_t*>(state.outgoing.data())};state.send_pending=true;
        const auto handle=state.handle;
        // Server sends run inside the serialized connection callback; release the
        // observation mutex because completion may be inline. Main-thread client
        // submissions retain it until the asynchronous API has accepted the handle.
        if(state.peer)lock.unlock();
        const auto status=api->StreamSend(handle,&state.buffer,1,QUIC_SEND_FLAG_FIN,&state);
        if(!lock.owns_lock())lock.lock();
        if(QUIC_FAILED(status)){state.send_pending=false;failed=true;changed.notify_all();}
    }
    static QUIC_STATUS QUIC_API stream_event(HQUIC handle,void* context,QUIC_STREAM_EVENT* event) {
        auto& s=*static_cast<stream*>(context);auto& self=*s.owner;
        try {
            switch(event->Type) {
            case QUIC_STREAM_EVENT_START_COMPLETE:{
                std::lock_guard lock(self.mutex);if(QUIC_FAILED(event->START_COMPLETE.Status))self.failed=true;
                else{s.started=true;s.id=event->START_COMPLETE.ID;}self.changed.notify_all();break;}
            case QUIC_STREAM_EVENT_RECEIVE:{
                std::lock_guard lock(self.mutex);
                if(event->RECEIVE.TotalBufferLength>65536-s.incoming.size()){self.failed=true;self.changed.notify_all();return QUIC_STATUS_ABORTED;}
                for(std::uint32_t i=0;i<event->RECEIVE.BufferCount;++i)s.incoming.append(reinterpret_cast<const char*>(event->RECEIVE.Buffers[i].Buffer),event->RECEIVE.Buffers[i].Length);
                const bool pend=event->RECEIVE.TotalBufferLength && s.peer && !s.release_receive && (s.uni || (s.id==0 && !self.resume_large));
                if(pend)s.pending=event->RECEIVE.TotalBufferLength;
                self.changed.notify_all();return pend?QUIC_STATUS_PENDING:QUIC_STATUS_SUCCESS;}
            case QUIC_STREAM_EVENT_SEND_COMPLETE:{
                std::lock_guard lock(self.mutex);
                if(event->SEND_COMPLETE.ClientContext!=&s || !s.send_pending)self.failed=true;
                s.send_pending=false;s.send_canceled=event->SEND_COMPLETE.Canceled!=FALSE;self.changed.notify_all();break;}
            case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
                if(s.peer && !s.uni)self.send(s,"ack");
                break;
            case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:{
                std::uint64_t pending=0;
                {std::lock_guard lock(self.mutex);s.aborted=true;s.release_receive=true;pending=std::exchange(s.pending,0);self.changed.notify_all();}
                if(pending)self.api->StreamReceiveComplete(handle,pending);
                break;}
            case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
                {std::lock_guard lock(self.mutex);s.handle=nullptr;}
                self.api->StreamClose(handle);
                {std::lock_guard lock(self.mutex);s.done=true;self.changed.notify_all();}
                break;
            default:break;
            }
            return QUIC_STATUS_SUCCESS;
        }catch(...){std::lock_guard lock(self.mutex);self.failed=true;self.changed.notify_all();return QUIC_STATUS_OUT_OF_MEMORY;}
    }
    static QUIC_STATUS QUIC_API connection_event(HQUIC handle,void* context,QUIC_CONNECTION_EVENT* event) {
        auto& c=*static_cast<connection*>(context);auto& self=*c.owner;
        switch(event->Type) {
        case QUIC_CONNECTION_EVENT_CONNECTED:{
            std::lock_guard lock(self.mutex);c.connected=true;
            if(event->CONNECTED.NegotiatedAlpnLength!=sizeof(protocol)-1 || std::memcmp(event->CONNECTED.NegotiatedAlpn,protocol,sizeof(protocol)-1))self.failed=true;
            self.changed.notify_all();break;}
        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:{std::lock_guard lock(self.mutex);c.transport=event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status;break;}
        case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:{
            stream* state=nullptr;
            {
                std::lock_guard lock(self.mutex);
                if(!c.peer || self.peer_count==self.peers.size()){self.failed=true;self.changed.notify_all();return QUIC_STATUS_OUT_OF_MEMORY;}
                state=&self.peers[self.peer_count++];state->handle=event->PEER_STREAM_STARTED.Stream;state->peer=true;
                state->uni=(event->PEER_STREAM_STARTED.Flags&QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL)!=0;
            }
            self.api->SetCallbackHandler(state->handle,reinterpret_cast<void*>(stream_event),state);
            std::uint32_t size=sizeof(state->id);std::uint64_t id=0;
            const auto status=self.api->GetParam(state->handle,QUIC_PARAM_STREAM_ID,&size,&id);
            {std::lock_guard lock(self.mutex);state->id=id;if(QUIC_FAILED(status))self.failed=true;self.changed.notify_all();}
            break;}
        case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
            {std::lock_guard lock(self.mutex);c.handle=nullptr;}
            self.api->ConnectionClose(handle);
            {std::lock_guard lock(self.mutex);c.done=true;self.changed.notify_all();}
            break;
        default:break;
        }
        return QUIC_STATUS_SUCCESS;
    }
    static QUIC_STATUS QUIC_API listener_event(HQUIC,void* context,QUIC_LISTENER_EVENT* event) {
        auto& self=*static_cast<experiment*>(context);
        if(event->Type!=QUIC_LISTENER_EVENT_NEW_CONNECTION)return QUIC_STATUS_SUCCESS;
        {std::lock_guard lock(self.mutex);if(self.server.handle)return QUIC_STATUS_CONNECTION_REFUSED;self.server.handle=event->NEW_CONNECTION.Connection;}
        self.api->SetCallbackHandler(event->NEW_CONNECTION.Connection,reinterpret_cast<void*>(connection_event),&self.server);
        const auto status=self.api->ConnectionSetConfiguration(event->NEW_CONNECTION.Connection,self.server_config);
        if(QUIC_FAILED(status)){std::lock_guard lock(self.mutex);self.server.handle=nullptr;self.failed=true;self.changed.notify_all();}
        return status; // Failed admission is closed by MsQuic, never by this callback.
    }
    explicit experiment(bool trusted=true) {
        for(auto& s:clients){s.owner=this;s.incoming.reserve(65536);s.outgoing.reserve(65536);}
        for(auto& s:peers){s.owner=this;s.incoming.reserve(65536);s.outgoing.reserve(65536);}
        c11::certificates::authority authority,other("Unrelated C11 CA");auto identity=authority.issue("localhost");
        directory=std::filesystem::temp_directory_path()/("c11-quic-"+std::to_string(c11::clock::now().time_since_epoch().count()));
        check(std::filesystem::create_directory(directory),"private QUIC certificate fixture directory");
        cert_file=(directory/"server.pem").string();key_file=(directory/"server.key").string();ca_file=(directory/"ca.pem").string();
        for(const auto& item:{std::pair{cert_file,identity.certificate},std::pair{key_file,identity.key},std::pair{ca_file,trusted?authority.certificate():other.certificate()}}){
            std::ofstream output(item.first,std::ios::binary);output<<item.second;check(static_cast<bool>(output),"private QUIC fixture file written");
        }
        good(MsQuicOpen2(&api),"MsQuic API2 opened");
        std::array<std::uint32_t,4> version{};std::uint32_t version_size=sizeof(version);
        good(api->GetParam(nullptr,QUIC_PARAM_GLOBAL_LIBRARY_VERSION,&version_size,version.data()),"MsQuic runtime version probed");
        check(version[0]==2 && version[1]==6 && version[2]==1,"loaded MsQuic matches fixed version");
        QUIC_REGISTRATION_CONFIG config{"LearnCPP-C11",QUIC_EXECUTION_PROFILE_TYPE_SCAVENGER};good(api->RegistrationOpen(&config,&registration),"QUIC registration");
        QUIC_SETTINGS settings{};
        settings.IsSet.IdleTimeoutMs=TRUE;settings.IdleTimeoutMs=8000;
        settings.IsSet.HandshakeIdleTimeoutMs=TRUE;settings.HandshakeIdleTimeoutMs=5000;
        settings.IsSet.PeerBidiStreamCount=TRUE;settings.PeerBidiStreamCount=3;
        settings.IsSet.PeerUnidiStreamCount=TRUE;settings.PeerUnidiStreamCount=1;
        settings.IsSet.StreamRecvWindowDefault=TRUE;settings.StreamRecvWindowDefault=4096;
        settings.IsSet.ConnFlowControlWindow=TRUE;settings.ConnFlowControlWindow=65536;
        settings.IsSet.SendBufferingEnabled=TRUE;settings.SendBufferingEnabled=FALSE;
        good(api->ConfigurationOpen(registration,&alpn,1,&settings,sizeof(settings),nullptr,&server_config),"server QUIC configuration");
        good(api->ConfigurationOpen(registration,&alpn,1,&settings,sizeof(settings),nullptr,&client_config),"client QUIC configuration");
        QUIC_CERTIFICATE_FILE files{key_file.c_str(),cert_file.c_str()};
        QUIC_CREDENTIAL_CONFIG server_credentials{};server_credentials.Type=QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;server_credentials.CertificateFile=&files;
        good(api->ConfigurationLoadCredential(server_config,&server_credentials),"QUIC server certificate loaded");
        QUIC_CREDENTIAL_CONFIG client_credentials{};client_credentials.Type=QUIC_CREDENTIAL_TYPE_NONE;
        client_credentials.Flags=static_cast<QUIC_CREDENTIAL_FLAGS>(QUIC_CREDENTIAL_FLAG_CLIENT|QUIC_CREDENTIAL_FLAG_SET_CA_CERTIFICATE_FILE|QUIC_CREDENTIAL_FLAG_USE_TLS_BUILTIN_CERTIFICATE_VALIDATION);
        client_credentials.CaCertificateFile=ca_file.c_str();good(api->ConfigurationLoadCredential(client_config,&client_credentials),"QUIC verifies against private CA");
        good(api->ListenerOpen(registration,listener_event,this,&listener),"QUIC listener opened");
        QUIC_ADDR address{};QuicAddrSetFamily(&address,QUIC_ADDRESS_FAMILY_INET);QuicAddrSetToLoopback(&address);QuicAddrSetPort(&address,0);
        good(api->ListenerStart(listener,&alpn,1,&address),"QUIC loopback UDP listener starts");
        std::uint32_t size=sizeof(address);good(api->GetParam(listener,QUIC_PARAM_LISTENER_LOCAL_ADDRESS,&size,&address),"QUIC UDP port observed");
        good(api->ConnectionOpen(registration,connection_event,&client,&client.handle),"QUIC client opened");
        good(api->ConnectionStart(client.handle,client_config,QUIC_ADDRESS_FAMILY_INET,"localhost",QuicAddrGetPort(&address)),"QUIC TLS handshake started");
    }
    stream* peer(std::uint64_t id){for(auto& s:peers)if(s.id==id)return &s;return nullptr;}
    void open(std::size_t index,bool uni=false) {
        auto& s=clients[index];std::unique_lock lock(mutex);s.uni=uni;
        check(client.handle!=nullptr,"connection remains owned before opening stream");
        good(api->StreamOpen(client.handle,uni?QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL:QUIC_STREAM_OPEN_FLAG_NONE,stream_event,&s,&s.handle),"QUIC stream opened");
        good(api->StreamStart(s.handle,QUIC_STREAM_START_FLAG_IMMEDIATE),"QUIC stream start accepted");lock.unlock();wait([&]{return s.started;});
    }
    bool close() {
        if(closed)return true;closed=true;
        if(listener){api->ListenerClose(listener);listener=nullptr;}
        if(registration){
            {std::lock_guard lock(mutex);resume_large=true;for(auto& s:peers){s.release_receive=true;if(s.handle && s.pending){auto n=std::exchange(s.pending,0);api->StreamReceiveComplete(s.handle,n);}}}
            api->RegistrationShutdown(registration,QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT,0);
            if(server_config)api->ConfigurationClose(server_config);
            if(client_config)api->ConfigurationClose(client_config);
            api->RegistrationClose(registration);registration=nullptr;
        }
        if(api){MsQuicClose(api);api=nullptr;}
        std::error_code error;
        for(const auto& file:{cert_file,key_file,ca_file}){std::filesystem::remove(file,error);if(error)return false;}
        std::filesystem::remove(directory,error);return !error;
    }
    ~experiment(){if(!closed)(void)close();}
};
int main() {
    c11::socket_runtime runtime;
    {
        experiment test;test.wait([&]{return test.client.connected&&test.server.connected;});
        test.open(0);test.open(1);check(test.clients[0].id==0 && test.clients[1].id==4,"QUIC bidirectional stream identifiers");
        test.send(test.clients[0],std::string(32768,'L'));test.send(test.clients[1],"small");
        test.wait([&]{auto* large=test.peer(0);return large&&large->pending>0&&test.clients[1].done;});
        {std::lock_guard lock(test.mutex);check(test.clients[0].send_pending,"withheld stream credit keeps large send buffer in flight");check(test.clients[1].incoming=="ack","other stream completes while first receive is pending");
            auto* large=test.peer(0);test.resume_large=true;auto length=std::exchange(large->pending,0);test.api->StreamReceiveComplete(large->handle,length);}
        test.wait([&]{return test.clients[0].done;});
        {std::lock_guard lock(test.mutex);check(test.peer(0)->incoming==std::string(32768,'L') && test.clients[0].incoming=="ack","resumed receive consumes full authenticated stream");}
        test.open(2,true);test.send(test.clients[2],std::string(32768,'C'));
        test.wait([&]{auto* stream=test.peer(test.clients[2].id);return stream&&stream->pending>0;});
        {std::lock_guard lock(test.mutex);experiment::good(test.api->StreamShutdown(test.clients[2].handle,QUIC_STREAM_SHUTDOWN_FLAG_ABORT_SEND,42),"QUIC application abort submitted");}
        test.wait([&]{auto* stream=test.peer(test.clients[2].id);return test.clients[2].done&&stream&&stream->done;});
        {std::lock_guard lock(test.mutex);check(test.peer(test.clients[2].id)->aborted && test.clients[2].send_canceled,"peer reset and canceled send completion observed");
            for(const auto& stream:test.clients)check(!stream.send_pending,"all application send buffers released only after completion");
            test.api->ConnectionShutdown(test.client.handle,QUIC_CONNECTION_SHUTDOWN_FLAG_NONE,0);}
        test.wait([&]{return test.client.done&&test.server.done;});check(test.close(),"QUIC registration and private fixture cleanup");
    }
    {
        experiment untrusted(false);untrusted.wait([&]{return untrusted.client.done;});
        std::cout<<"untrusted QUIC peer status=0x"<<std::hex<<static_cast<std::uint32_t>(untrusted.client.transport)<<std::dec<<'\n';
        check(!untrusted.client.connected && untrusted.client.transport==QUIC_STATUS_TLS_ALERT(48),"QUIC rejects an unknown CA without disabling validation");
        check(untrusted.close(),"rejected QUIC handshake cleanup");
    }
    std::cout<<"QUIC TLS, dual streams, receive backpressure, reset, send lifetime and connection shutdown passed\n";
}
