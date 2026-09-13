#pragma once
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include <memory>
#include <stdexcept>
#include <string>

namespace c11::certificates {
using key=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
using certificate=std::unique_ptr<X509,decltype(&X509_free)>;
using bio=std::unique_ptr<BIO,decltype(&BIO_free)>;
inline void require(bool success){if(!success)throw std::runtime_error("test certificate construction failed");}
inline key make_key() {
    std::unique_ptr<EVP_PKEY_CTX,decltype(&EVP_PKEY_CTX_free)> context(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA,nullptr),EVP_PKEY_CTX_free);
    require(context && EVP_PKEY_keygen_init(context.get())>0 && EVP_PKEY_CTX_set_rsa_keygen_bits(context.get(),2048)>0);
    EVP_PKEY* raw=nullptr;require(EVP_PKEY_keygen(context.get(),&raw)>0);return key(raw,EVP_PKEY_free);
}
inline void extension(X509* cert,X509* issuer,int id,const char* value) {
    X509V3_CTX context{};X509V3_set_ctx(&context,issuer,cert,nullptr,nullptr,0);
    std::unique_ptr<X509_EXTENSION,decltype(&X509_EXTENSION_free)> item(X509V3_EXT_nconf_nid(nullptr,&context,id,value),X509_EXTENSION_free);
    require(item && X509_add_ext(cert,item.get(),-1)==1);
}
inline certificate make_certificate(EVP_PKEY* subject_key,X509* issuer,EVP_PKEY* signer,const std::string& hostname,bool ca,bool expired,bool client) {
    certificate cert(X509_new(),X509_free);require(static_cast<bool>(cert));
    require(X509_set_version(cert.get(),2)==1 && ASN1_INTEGER_set(X509_get_serialNumber(cert.get()),ca?1:2)==1);
    require(X509_gmtime_adj(X509_get_notBefore(cert.get()),-3600)!=nullptr);
    require(X509_gmtime_adj(X509_get_notAfter(cert.get()),expired?-60:3600)!=nullptr);
    require(X509_set_pubkey(cert.get(),subject_key)==1);
    auto* name=X509_get_subject_name(cert.get());
    require(X509_NAME_add_entry_by_txt(name,"CN",MBSTRING_ASC,reinterpret_cast<const unsigned char*>(hostname.c_str()),-1,-1,0)==1);
    require(X509_set_issuer_name(cert.get(),issuer?X509_get_subject_name(issuer):name)==1);
    extension(cert.get(),issuer?issuer:cert.get(),NID_basic_constraints,ca?"critical,CA:TRUE,pathlen:0":"critical,CA:FALSE");
    extension(cert.get(),issuer?issuer:cert.get(),NID_key_usage,ca?"critical,keyCertSign,cRLSign":"critical,digitalSignature,keyEncipherment");
    if(!ca) {
        extension(cert.get(),issuer,NID_ext_key_usage,client?"clientAuth":"serverAuth");
        extension(cert.get(),issuer,NID_subject_alt_name,("DNS:"+hostname).c_str());
    }
    require(X509_sign(cert.get(),signer,EVP_sha256())>0);return cert;
}
inline std::string pem(X509* cert) {
    bio memory(BIO_new(BIO_s_mem()),BIO_free);require(memory && PEM_write_bio_X509(memory.get(),cert)==1);
    char* data=nullptr;const auto size=BIO_get_mem_data(memory.get(),&data);require(size>0);return {data,static_cast<std::size_t>(size)};
}
inline std::string pem(EVP_PKEY* key) {
    bio memory(BIO_new(BIO_s_mem()),BIO_free);require(memory && PEM_write_bio_PrivateKey(memory.get(),key,nullptr,nullptr,0,nullptr,nullptr)==1);
    char* data=nullptr;const auto size=BIO_get_mem_data(memory.get(),&data);require(size>0);return {data,static_cast<std::size_t>(size)};
}
struct identity {std::string certificate,key;};
// Test-only CA: keys remain process-local and never enter the OS trust store.
class authority {
    certificates::key key_=make_key();
    certificates::certificate certificate_;
public:
    explicit authority(const std::string& name="C11 test CA")
        :certificate_(make_certificate(key_.get(),nullptr,key_.get(),name,true,false,false)){}
    std::string certificate() const{return pem(certificate_.get());}
    identity issue(const std::string& hostname,bool expired=false,bool client=false) const {
        auto key=make_key();auto leaf=make_certificate(key.get(),certificate_.get(),key_.get(),hostname,false,expired,client);
        return {pem(leaf.get()),pem(key.get())};
    }
};
}
