#include <c11/socket.hpp>
#include <check.hpp>
#include <array>
#include <iostream>
#include <type_traits>
using namespace std::chrono_literals;
struct operation { OVERLAPPED overlapped{}; std::array<char,64> buffer{}; bool pending=false; };
static_assert(std::is_standard_layout_v<operation>);
struct port_owner {HANDLE handle;~port_owner(){if(handle) CloseHandle(handle);}};

void receive(c11::socket_handle socket,operation& op) {
    check(!op.pending,"operation identity not reused before final completion");
    op.overlapped={};WSABUF buffer{static_cast<ULONG>(op.buffer.size()),op.buffer.data()};
    DWORD flags=0,bytes=0;
    const int result=WSARecv(socket,&buffer,1,&bytes,&flags,&op.overlapped,nullptr);
    check(result==0 || WSAGetLastError()==WSA_IO_PENDING,"WSARecv submitted or completed inline");
    // Default IOCP notification mode: even immediate success produces a packet.
    op.pending=true;
}
struct completion {operation* target;DWORD bytes;DWORD error;};
completion next(HANDLE port) {
    DWORD bytes=0;ULONG_PTR key=0;OVERLAPPED* raw=nullptr;
    const BOOL success=GetQueuedCompletionStatus(port,&bytes,&key,&raw,5000);
    const DWORD error=success?ERROR_SUCCESS:GetLastError();
    check(raw!=nullptr,"a target completion arrives before the external timeout");
    auto* op=reinterpret_cast<operation*>(raw);
    check(op->pending,"completion consumes one accepted operation");
    check(key==0xC11,"completion key identifies this socket binding");
    op->pending=false;
    return {op,bytes,error};
}
int main() {
    c11::socket_runtime runtime;
    auto listener=c11::listen_loopback();check(listener.has_value(),"IOCP listener");
    const auto end=c11::clock::now()+5s;
    auto client=c11::connect_loopback(listener->port,end);check(client.has_value(),"IOCP client");
    check(c11::wait_ready(listener->socket.get(),POLLRDNORM,end).has_value(),"IOCP accept ready");
    auto socket=c11::accept_socket(listener->socket.get());check(socket.has_value(),"IOCP accepted overlapped socket");
    port_owner port{CreateIoCompletionPort(INVALID_HANDLE_VALUE,nullptr,0,1)};
    check(port.handle!=nullptr,"completion port created");
    check(CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket->get()),port.handle,0xC11,0)==port.handle,"socket associated with port");
    operation read;
    receive(socket->get(),read);
    check(c11::send_all(client->get(),std::string_view("hello"),end).has_value(),"IOCP payload sent");
    auto done=next(port.handle);
    check(done.target==&read && done.error==0 && done.bytes>0 && done.bytes<=5,"actual receive completion identity and bytes");
    std::string payload(read.buffer.data(),done.bytes);
    while(payload.size()<5) {
        receive(socket->get(),read);done=next(port.handle);
        check(done.error==0 && done.bytes>0,"remaining receive completes");payload.append(read.buffer.data(),done.bytes);
    }
    check(payload=="hello","short completion fragments reconstruct payload");
    operation write;
    std::copy(payload.begin(),payload.end(),write.buffer.begin());
    std::size_t offset=0;
    while(offset<payload.size()) {
        write.overlapped={};WSABUF data{static_cast<ULONG>(payload.size()-offset),write.buffer.data()+offset};DWORD bytes=0;
        const int result=WSASend(socket->get(),&data,1,&bytes,0,&write.overlapped,nullptr);
        check(result==0 || WSAGetLastError()==WSA_IO_PENDING,"WSASend accepted");write.pending=true;
        done=next(port.handle);check(done.target==&write && done.error==0 && done.bytes>0 && done.bytes<=payload.size()-offset,"write completion progress");offset+=done.bytes;
    }
    auto response=c11::receive_exact(client->get(),5,end);check(response && *response=="hello","overlapped response reaches client");
    receive(socket->get(),read);
    const BOOL canceled=CancelIoEx(reinterpret_cast<HANDLE>(socket->get()),&read.overlapped);
    check(canceled || GetLastError()==ERROR_NOT_FOUND,"cancellation request result recorded separately");
    check(read.pending,"requesting cancellation does not release buffer");
    done=next(port.handle);
    check(done.target==&read && done.error==ERROR_OPERATION_ABORTED,"idle receive finally completes canceled");
    // Data is already available before submission; still collect exactly one packet.
    check(c11::send_all(client->get(),std::string_view("r"),end).has_value(),"race data queued");
    receive(socket->get(),read);
    const BOOL race_cancel=CancelIoEx(reinterpret_cast<HANDLE>(socket->get()),&read.overlapped);
    check(race_cancel || GetLastError()==ERROR_NOT_FOUND,"cancel/completion race request bounded");
    done=next(port.handle);
    check(done.target==&read && (done.error==0 || done.error==ERROR_OPERATION_ABORTED),"legal cancellation race outcomes");
    check(!read.pending && !write.pending,"every accepted operation has one final completion");
    std::cout<<"real IOCP recv/send/cancel and operation ownership passed\n";
}
