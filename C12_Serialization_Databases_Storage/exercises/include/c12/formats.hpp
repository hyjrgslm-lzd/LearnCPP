#pragma once
#include <c12/bytes.hpp>
#include <c05/manifest.hpp>
#include <boost/json.hpp>
#include <charconv>
#include <manifest_v1.pb.h>
#include <manifest_v2.pb.h>

namespace c12::formats {
inline void validate(const c05::Manifest& value){
    auto valid=c05::validate_manifest(value);
    if(!valid)throw failure(errc::invalid,valid.error().field.c_str(),0,valid.error().offset);
}
template<class Wire> Wire to_wire(const c05::Manifest& value){
    validate(value);Wire wire;
    for(const auto& input:value.records){
        auto* record=wire.add_records();
        record->set_id(input.id);record->set_name(input.name);record->set_path(input.path);
        record->set_byte_size(input.byte_size);record->set_modified_at_ms(input.modified_at_ms);
        if constexpr(requires{record->set_note(std::string{});}){
            if(input.note)record->set_note(*input.note);
        }else if(input.note)throw failure(errc::invalid,"old writer cannot represent note");
    }
    return wire;
}
template<class Wire> c05::Manifest from_wire(const Wire& wire){
    if(wire.records_size()>static_cast<int>(c05::max_records))throw failure(errc::capacity,"record count");
    c05::Manifest result;result.records.reserve(static_cast<std::size_t>(wire.records_size()));
    for(const auto& input:wire.records()){
        if(!input.has_id()||!input.has_name()||!input.has_path()||!input.has_byte_size()||!input.has_modified_at_ms())
            throw failure(errc::invalid,"required domain field is absent");
        c05::ResourceRecord record{input.id(),input.name(),input.path(),input.byte_size(),input.modified_at_ms(),{}};
        if constexpr(requires{input.has_note();})if(input.has_note())record.note=input.note();
        result.records.push_back(std::move(record));
    }
    validate(result);return result;
}
template<class Wire> std::string encode_proto(const c05::Manifest& value){
    auto wire=to_wire<Wire>(value);
    if(wire.ByteSizeLong()>c05::max_package_bytes)throw failure(errc::capacity,"protobuf byte limit");
    std::string output;
    if(!wire.SerializeToString(&output))throw failure(errc::corrupt,"protobuf serialization failed");
    return output;
}
template<class Wire> Wire parse_proto(std::string_view input){
    if(input.size()>c05::max_package_bytes)throw failure(errc::capacity,"protobuf byte limit");
    Wire wire;
    if(!wire.ParseFromArray(input.empty()?"":input.data(),static_cast<int>(input.size())))
        throw failure(errc::corrupt,"protobuf syntax rejected");
    return wire;
}
namespace json=boost::json;
inline json::value to_json(const c05::Manifest& value){
    validate(value);json::array records;
    for(const auto& record:value.records){
        json::object item{{"id",record.id},{"name",record.name},{"path",record.path},
            {"byte_size",std::to_string(record.byte_size)},{"modified_at_ms",record.modified_at_ms}};
        if(record.note)item["note"]=*record.note;
        records.push_back(std::move(item));
    }
    return json::object{{"schema",1},{"records",std::move(records)}};
}
inline const json::value& member(const json::object& object,json::string_view key){
    auto* value=object.if_contains(key);if(!value)throw failure(errc::invalid,"missing JSON member");return *value;
}
inline std::uint64_t unsigned_number(const json::value& value){
    if(value.is_uint64())return value.as_uint64();
    if(value.is_int64()&&value.as_int64()>=0)return static_cast<std::uint64_t>(value.as_int64());
    throw failure(errc::invalid,"unsigned JSON integer required");
}
inline std::int64_t signed_number(const json::value& value){
    if(value.is_int64())return value.as_int64();
    if(value.is_uint64()&&value.as_uint64()<=INT64_MAX)return static_cast<std::int64_t>(value.as_uint64());
    throw failure(errc::invalid,"signed JSON integer required");
}
inline std::string string(const json::value& value,std::size_t maximum=c05::max_text_bytes){
    if(!value.is_string()||value.as_string().size()>maximum)throw failure(errc::invalid,"bounded JSON string required");
    return {value.as_string().data(),value.as_string().size()};
}
inline c05::Manifest from_json(const json::value& value){
    if(!value.is_object())throw failure(errc::invalid,"JSON object required");
    const auto& object=value.as_object();
    if(unsigned_number(member(object,"schema"))!=1)throw failure(errc::invalid,"JSON schema version");
    const auto& list=member(object,"records");
    if(!list.is_array()||list.as_array().size()>c05::max_records)throw failure(errc::capacity,"JSON record count");
    c05::Manifest result;
    for(const auto& value:list.as_array()){
        if(!value.is_object())throw failure(errc::invalid,"record object required");
        const auto& item=value.as_object();const auto id=unsigned_number(member(item,"id"));
        if(id>UINT32_MAX)throw failure(errc::invalid,"resource id width");
        const auto decimal=string(member(item,"byte_size"),20);std::uint64_t byte_size=0;
        const auto [end,error]=std::from_chars(decimal.data(),decimal.data()+decimal.size(),byte_size);
        if(error!=std::errc{}||end!=decimal.data()+decimal.size())throw failure(errc::invalid,"complete uint64 decimal string required");
        c05::ResourceRecord record{static_cast<std::uint32_t>(id),string(member(item,"name")),string(member(item,"path")),
                                   byte_size,signed_number(member(item,"modified_at_ms")),{}};
        if(auto* note=item.if_contains("note"))record.note=string(*note);
        result.records.push_back(std::move(record));
    }
    validate(result);return result;
}
inline std::string encode_json(const c05::Manifest& value){
    auto output=json::serialize(to_json(value));
    if(output.size()>c05::max_package_bytes)throw failure(errc::capacity,"JSON byte limit");
    return output;
}
inline c05::Manifest decode_json(std::string_view input){
    if(input.size()>c05::max_package_bytes)throw failure(errc::capacity,"JSON byte limit");
    json::parse_options options;options.max_depth=8;
    boost::system::error_code error;
    auto value=json::parse(json::string_view(input.data(),input.size()),error,{},options);
    if(error)throw failure(errc::corrupt,"JSON syntax rejected");
    return from_json(value);
}
}
