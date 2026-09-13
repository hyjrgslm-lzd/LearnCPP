#include "service.hpp"
#include <c11/certificate_fixture.hpp>
#include <grpc/impl/channel_arg_names.h>
#include <check.hpp>
#include <iostream>
using namespace std::chrono_literals;
int run_checks() {
    c11::certificates::authority ca;auto server_identity=ca.issue("localhost");
    boost::asio::io_context owner;std::promise<void> gate;
    auto engine=std::make_shared<c11::grpc_service::runtime>(owner,c11::tasks::limits{},gate.get_future().share());engine->start();
    std::exception_ptr owner_error;std::jthread io([&]{try{owner.run();}catch(...){owner_error=std::current_exception();}});
    c11::grpc_service::service facade(owner,engine);
    grpc::SslServerCredentialsOptions tls;tls.pem_key_cert_pairs.push_back({server_identity.key,server_identity.certificate});
    grpc::ServerBuilder builder;int port=0;
    builder.AddListeningPort("127.0.0.1:0",grpc::SslServerCredentials(tls),&port);builder.RegisterService(&facade);
    builder.SetMaxReceiveMessageSize(4096);builder.SetMaxSendMessageSize(4096);
    builder.AddChannelArgument(GRPC_ARG_MAX_CONCURRENT_STREAMS,16);
    grpc::ResourceQuota quota;quota.Resize(64*1024*1024).SetMaxThreads(64);builder.SetResourceQuota(quota);
    auto server=builder.BuildAndStart();check(server && port>0,"private TLS gRPC server starts");
    struct cleanup {
        boost::asio::io_context& owner;std::shared_ptr<c11::grpc_service::runtime> engine;std::promise<void>& gate;std::jthread& io;std::unique_ptr<grpc::Server>& server;
        ~cleanup(){if(io.joinable()){
            try{gate.set_value();}catch(const std::future_error&){}
            if(server){server->Shutdown(std::chrono::system_clock::now()+5s);server->Wait();}
            try{boost::asio::post(owner,[engine=engine]{engine->stop([]{});});}catch(...){owner.stop();}
            io.join();
        }}
    } cleanup_guard{owner,engine,gate,io,server};
    grpc::SslCredentialsOptions client_tls;client_tls.pem_root_certs=ca.certificate();
    grpc::ChannelArguments args;args.SetSslTargetNameOverride("localhost");args.SetMaxReceiveMessageSize(4096);
    auto channel=grpc::CreateCustomChannel("127.0.0.1:"+std::to_string(port),grpc::SslCredentials(client_tls),args);
    auto stub=c11::rpc::Tasks::NewStub(channel);
    c11::rpc::SubmitRequest request;request.set_idempotency_key("grpc-key");request.set_left(20);request.set_right(22);request.set_steps(3);request.set_budget_ms(5000);
    grpc::ClientContext first;first.set_deadline(std::chrono::system_clock::now()+3s);c11::rpc::TaskSnapshot created;
    check(stub->Submit(&first,request,&created).ok(),"unary Submit over real HTTP2/TLS");
    check(first.auth_context() && first.auth_context()->IsPeerAuthenticated(),"gRPC client authenticated server certificate");
    grpc::ClientContext duplicate;duplicate.set_deadline(std::chrono::system_clock::now()+3s);c11::rpc::TaskSnapshot retried;
    check(stub->Submit(&duplicate,request,&retried).ok() && retried.id()==created.id(),"gRPC retry reuses task identity");
    request.set_left(21);grpc::ClientContext conflict;conflict.set_deadline(std::chrono::system_clock::now()+3s);
    check(stub->Submit(&conflict,request,&retried).error_code()==grpc::StatusCode::ALREADY_EXISTS,"gRPC conflict channel");
    c11::rpc::TaskId id;id.set_id(created.id());grpc::ClientContext watch;watch.set_deadline(std::chrono::system_clock::now()+4s);
    auto reader=stub->Watch(&watch,id);c11::rpc::TaskSnapshot event;
    check(reader->Read(&event) && event.state()=="running","server stream starts with current snapshot");
    grpc::ClientContext cancel;cancel.set_deadline(std::chrono::system_clock::now()+3s);
    check(stub->Cancel(&cancel,id,&retried).ok() && retried.stop_requested(),"gRPC cancellation requests cooperative stop");
    gate.set_value();bool terminal=false;
    while(reader->Read(&event))if(event.state()=="cancelled")terminal=true;
    check(reader->Finish().ok() && terminal,"server stream closes after terminal cancellation");
    grpc::ClientContext expired;expired.set_deadline(std::chrono::system_clock::time_point{});
    const auto expired_status=stub->Get(&expired,id,&retried);
    std::cout<<"expired RPC status="<<expired_status.error_code()<<" "<<expired_status.error_message()<<'\n';
    check(expired_status.error_code()==grpc::StatusCode::DEADLINE_EXCEEDED,"client deadline remains distinct from task budget");
    grpc::ChannelArguments wrong;wrong.SetSslTargetNameOverride("wrong.test");
    auto wrong_stub=c11::rpc::Tasks::NewStub(grpc::CreateCustomChannel("127.0.0.1:"+std::to_string(port),grpc::SslCredentials(client_tls),wrong));
    grpc::ClientContext wrong_name;wrong_name.set_deadline(std::chrono::system_clock::now()+500ms);
    check(!wrong_stub->Get(&wrong_name,id,&retried).ok(),"gRPC TLS does not bypass hostname validation");
    server->Shutdown(std::chrono::system_clock::now()+5s);server->Wait();
    boost::asio::post(owner,[engine]{engine->stop([]{});});io.join();if(owner_error)std::rethrow_exception(owner_error);
    check(engine->registry.live()==0 && engine->registry.subscriptions()==0 && engine->joined() && !engine->deadline_exceeded && !engine->failure,"gRPC calls, subscriptions, owner and workers converge");
    std::cout<<"gRPC unary/server-streaming, protobuf, TLS, deadline and shared domain contracts passed\n";return 0;
}
int main(){try{return run_checks();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
