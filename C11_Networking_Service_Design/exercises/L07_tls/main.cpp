#include <c11/certificate_fixture.hpp>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <check.hpp>
#include <array>
#include <iostream>
using context=std::unique_ptr<SSL_CTX,decltype(&SSL_CTX_free)>;
using stream=std::unique_ptr<SSL,decltype(&SSL_free)>;

void identity(SSL_CTX* context,const c11::certificates::identity& identity) {
    c11::certificates::bio cert(BIO_new_mem_buf(identity.certificate.data(),static_cast<int>(identity.certificate.size())),BIO_free);
    c11::certificates::bio key(BIO_new_mem_buf(identity.key.data(),static_cast<int>(identity.key.size())),BIO_free);
    c11::certificates::certificate x(PEM_read_bio_X509(cert.get(),nullptr,nullptr,nullptr),X509_free);
    c11::certificates::key k(PEM_read_bio_PrivateKey(key.get(),nullptr,nullptr,nullptr),EVP_PKEY_free);
    check(x && k && SSL_CTX_use_certificate(context,x.get())==1 && SSL_CTX_use_PrivateKey(context,k.get())==1,"identity loaded from process-local PEM");
}
void trust(SSL_CTX* context,const std::string& root) {
    c11::certificates::bio pem(BIO_new_mem_buf(root.data(),static_cast<int>(root.size())),BIO_free);
    c11::certificates::certificate ca(PEM_read_bio_X509(pem.get(),nullptr,nullptr,nullptr),X509_free);
    check(ca && X509_STORE_add_cert(SSL_CTX_get_cert_store(context),ca.get())==1,"private trust root loaded");
}
struct outcome {bool connected;long verify;};
outcome handshake(const c11::certificates::identity& server_id,const std::string& root,const char* name,bool mutual=false,const c11::certificates::identity* client_id=nullptr) {
    context server_context(SSL_CTX_new(TLS_server_method()),SSL_CTX_free),client_context(SSL_CTX_new(TLS_client_method()),SSL_CTX_free);
    check(server_context && client_context,"TLS contexts created");
    check(SSL_CTX_set_min_proto_version(server_context.get(),TLS1_3_VERSION)==1 && SSL_CTX_set_min_proto_version(client_context.get(),TLS1_3_VERSION)==1,"TLS1.3 minimum configured");
    identity(server_context.get(),server_id);trust(client_context.get(),root);SSL_CTX_set_verify(client_context.get(),SSL_VERIFY_PEER,nullptr);
    if(mutual){trust(server_context.get(),root);SSL_CTX_set_verify(server_context.get(),SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT,nullptr);}
    if(client_id)identity(client_context.get(),*client_id);
    stream client(SSL_new(client_context.get()),SSL_free),server(SSL_new(server_context.get()),SSL_free);
    check(client && server && SSL_set1_host(client.get(),name)==1,"hostname verification configured separately from routing");
    check(SSL_set_tlsext_host_name(client.get(),name)==1,"SNI configured");
    BIO *left=nullptr,*right=nullptr;check(BIO_new_bio_pair(&left,4096,&right,4096)==1,"bounded TLS transport BIO pair");
    SSL_set_bio(client.get(),left,left);SSL_set_bio(server.get(),right,right);
    SSL_set_connect_state(client.get());SSL_set_accept_state(server.get());
    bool client_done=false,server_done=false,failed=false;
    for(unsigned attempts=0;attempts<1000 && !(client_done&&server_done) && !failed;++attempts) {
        for(auto pair:{std::pair{client.get(),&client_done},std::pair{server.get(),&server_done}}) {
            if(*pair.second)continue;
            ERR_clear_error();const int result=SSL_do_handshake(pair.first);
            const int error=SSL_get_error(pair.first,result);
            if(result==1)*pair.second=true;
            else if(error!=SSL_ERROR_WANT_READ && error!=SSL_ERROR_WANT_WRITE){failed=true;break;}
        }
    }
    if(!client_done || !server_done)return {false,SSL_get_verify_result(client.get())};
    check(SSL_version(client.get())==TLS1_3_VERSION,"actual protocol negotiated TLS1.3");
    const std::string text="encrypted payload";std::size_t sent=0,read=0;std::array<char,64> output{};
    check(SSL_write_ex(client.get(),text.data(),text.size(),&sent)==1 && sent==text.size(),"TLS record written");
    check(SSL_read_ex(server.get(),output.data(),output.size(),&read)==1 && std::string_view(output.data(),read)==text,"peer authenticates/decrypts actual TLS record");
    bool closed_client=false,closed_server=false;
    for(int step=0;step<20 && !(closed_client&&closed_server);++step) {
        for(auto pair:{std::pair{client.get(),&closed_client},std::pair{server.get(),&closed_server}}) {
            if(*pair.second)continue;
            ERR_clear_error();const int result=SSL_shutdown(pair.first);const int error=SSL_get_error(pair.first,result);
            if(result==1)*pair.second=true;
            else check(result==0 || error==SSL_ERROR_WANT_READ || error==SSL_ERROR_WANT_WRITE,"TLS close progresses or waits");
        }
    }
    check(closed_client && closed_server,"both close_notify messages consumed");
    return {true,SSL_get_verify_result(client.get())};
}
int main() {
    c11::certificates::authority ca,other("Unrelated C11 CA");
    const auto server=ca.issue("localhost");
    check(handshake(server,ca.certificate(),"localhost").connected,"trusted hostname connects");
    auto wrong_host=handshake(server,ca.certificate(),"wrong.test");
    check(!wrong_host.connected && wrong_host.verify==X509_V_ERR_HOSTNAME_MISMATCH,"hostname mismatch rejected");
    auto unknown=handshake(server,other.certificate(),"localhost");
    check(!unknown.connected && unknown.verify!=X509_V_OK,"unknown certificate authority rejected");
    auto expired=handshake(ca.issue("localhost",true),ca.certificate(),"localhost");
    check(!expired.connected && expired.verify==X509_V_ERR_CERT_HAS_EXPIRED,"expired certificate rejected");
    check(!handshake(server,ca.certificate(),"localhost",true).connected,"mutual TLS requires client identity");
    auto client=ca.issue("client.test",false,true);
    check(handshake(server,ca.certificate(),"localhost",true,&client).connected,"mutual TLS validates client chain and purpose");
    std::cout<<"TLS1.3 identity, encrypted records, mTLS and close_notify checks passed\n";
}
