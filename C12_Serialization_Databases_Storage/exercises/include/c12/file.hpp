#pragma once
#include <c12/bytes.hpp>
#include <filesystem>
#include <utility>
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace c12 {
class file {
#ifdef _WIN32
    HANDLE handle_=INVALID_HANDLE_VALUE;
    static int native_error(){return static_cast<int>(GetLastError());}
    static HANDLE invalid(){return INVALID_HANDLE_VALUE;}
#else
    int handle_=-1;
    static int native_error(){return errno;}
    static int invalid(){return -1;}
#endif
    [[noreturn]] static void failed(const char* op){throw failure(errc::io,op,native_error());}
public:
    file()=default;
    file(const file&)=delete;file& operator=(const file&)=delete;
    file(file&& other) noexcept:handle_(std::exchange(other.handle_,invalid())){}
    file& operator=(file&& other) noexcept {
        if(this!=&other){close();handle_=std::exchange(other.handle_,invalid());}return *this;
    }
    explicit file(const std::filesystem::path& path) {
#ifdef _WIN32
        handle_=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(handle_==invalid())failed("CreateFileW");
#else
        handle_=::open(path.c_str(),O_RDWR|O_CREAT|O_CLOEXEC,0600);
        if(handle_==invalid())failed("open");
        if(flock(handle_,LOCK_EX|LOCK_NB)){const int saved=errno;close();throw failure(errc::busy,"flock",saved);}
#endif
    }
    ~file(){close();}
    void close() noexcept {
        if(handle_==invalid())return;
#ifdef _WIN32
        CloseHandle(handle_);
#else
        ::close(handle_);
#endif
        handle_=invalid();
    }
    std::uint64_t size()const {
#ifdef _WIN32
        LARGE_INTEGER n{};if(!GetFileSizeEx(handle_,&n))failed("GetFileSizeEx");
        return static_cast<std::uint64_t>(n.QuadPart);
#else
        struct stat n{};if(fstat(handle_,&n))failed("fstat");return static_cast<std::uint64_t>(n.st_size);
#endif
    }
    void seek(std::uint64_t offset){
        if(offset>INT64_MAX)throw failure(errc::capacity,"file offset");
#ifdef _WIN32
        LARGE_INTEGER n{};n.QuadPart=static_cast<LONGLONG>(offset);
        if(!SetFilePointerEx(handle_,n,nullptr,FILE_BEGIN))failed("SetFilePointerEx");
#else
        if(lseek(handle_,static_cast<off_t>(offset),SEEK_SET)<0)failed("lseek");
#endif
    }
    bytes read_all(std::size_t limit){
        auto n=size();if(n>limit)throw failure(errc::capacity,"file limit");
        bytes out(static_cast<std::size_t>(n));seek(0);std::size_t done=0;
        while(done!=out.size()){
#ifdef _WIN32
            DWORD count=0;
            if(!ReadFile(handle_,out.data()+done,static_cast<DWORD>(out.size()-done),&count,nullptr))failed("ReadFile");
#else
            auto count=::read(handle_,out.data()+done,out.size()-done);
            if(count<0&&errno==EINTR)continue;if(count<0)failed("read");
#endif
            if(!count)throw failure(errc::io,"short file");
            done+=static_cast<std::size_t>(count);
        }return out;
    }
    void write_at(std::uint64_t offset,std::span<const std::byte> in){
        seek(offset);std::size_t done=0;
        while(done!=in.size()){
#ifdef _WIN32
            DWORD count=0;auto chunk=static_cast<DWORD>(std::min<std::size_t>(in.size()-done,1u<<20));
            if(!WriteFile(handle_,in.data()+done,chunk,&count,nullptr))failed("WriteFile");
#else
            auto count=::write(handle_,in.data()+done,in.size()-done);
            if(count<0&&errno==EINTR)continue;if(count<0)failed("write");
#endif
            if(!count)throw failure(errc::io,"zero write");
            done+=static_cast<std::size_t>(count);
        }
    }
    void truncate(std::uint64_t length){
#ifdef _WIN32
        seek(length);if(!SetEndOfFile(handle_))failed("SetEndOfFile");
#else
        if(length>INT64_MAX||ftruncate(handle_,static_cast<off_t>(length)))failed("ftruncate");
#endif
    }
    void flush(){
#ifdef _WIN32
        if(!FlushFileBuffers(handle_))failed("FlushFileBuffers");
#else
        if(fsync(handle_))failed("fsync");
#endif
    }
};
}
