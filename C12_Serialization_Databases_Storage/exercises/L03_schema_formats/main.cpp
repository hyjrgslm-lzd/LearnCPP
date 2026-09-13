#include <c12/formats.hpp>
#include <google/protobuf/arena.h>
#include <google/protobuf/util/json_util.h>
#include <check.hpp>
#include <iostream>
using V1=c12::pb::v1::Manifest;
using V2=c12::pb::v2::Manifest;
int main(){
    using namespace c12;
    try{
        const c05::Manifest model{{{7,"mesh","assets/a.mesh",UINT64_MAX,-1,std::string("future-note")}}};
        auto modern=formats::to_wire<V2>(model);
        auto encoded=formats::encode_proto<V2>(model);
        check(formats::from_wire(formats::parse_proto<V2>(encoded))==model,"real protobuf domain roundtrip");
        auto old=formats::parse_proto<V1>(encoded);
        const auto& old_record=old.records(0);
        check(old_record.GetReflection()->GetUnknownFields(old_record).field_count()==1,"old reader retains unknown note field");
        std::string forwarded;check(old.SerializeToString(&forwarded),"old binary forwarding");
        check(formats::from_wire(formats::parse_proto<V2>(forwarded))==model,"binary unknown-field forwarding preserves note");
        auto projected=formats::from_wire(old);
        check(!projected.records[0].note,"domain projection cannot retain undeclared field");
        auto projected_wire=formats::to_wire<V2>(projected);
        check(!projected_wire.records(0).has_note(),"domain reconstruction loses unknown fields");
        bool rejected=false;try{(void)formats::to_wire<V1>(model);}catch(const failure&){rejected=true;}
        check(rejected,"old writer must not silently discard a known note");
        auto zero=model;zero.records[0].byte_size=0;zero.records[0].note=std::string{};
        auto present=formats::to_wire<V2>(zero);
        check(present.records(0).has_byte_size()&&present.records(0).byte_size()==0&&present.records(0).has_note(),"explicit default and empty presence");
        present.mutable_records(0)->clear_byte_size();
        rejected=false;try{(void)formats::from_wire(present);}catch(const failure&){rejected=true;}
        check(rejected,"protobuf syntax-valid missing field can violate domain");
        std::string duplicate=modern.records(0).SerializeAsString();duplicate.push_back(char(8));duplicate.push_back(char(9));
        c12::pb::v2::Resource duplicated;
        check(duplicated.ParseFromString(duplicate)&&duplicated.id()==9,"protobuf singular field uses last value");
        const std::string malformed("\x0a\x05x",3);
        rejected=false;try{(void)formats::parse_proto<V2>(malformed);}catch(const failure&){rejected=true;}
        check(rejected,"declared protobuf length cannot exceed input");
        auto other=model;other.records[0].id=8;
        auto combined=formats::parse_proto<V2>(encoded+formats::encode_proto<V2>(other));
        check(formats::from_wire(combined).records.size()==2,"concatenation can merge messages: transport framing is separate");
        check(formats::decode_json(formats::encode_json(model))==model,"JSON profile roundtrip");
        auto json=formats::to_json(model);
        check(json.as_object().at("records").as_array()[0].as_object().at("byte_size").is_string(),"uint64 uses decimal string in JSON profile");
        auto bad=json;bad.as_object()["schema"]=2;
        rejected=false;try{(void)formats::from_json(bad);}catch(const failure&){rejected=true;}check(rejected,"unknown JSON major rejected");
        bad=json;bad.as_object()["records"].as_array()[0].as_object()["byte_size"]="18446744073709551616";
        rejected=false;try{(void)formats::from_json(bad);}catch(const failure&){rejected=true;}check(rejected,"JSON decimal overflow");
        bad=json;bad.as_object()["records"].as_array()[0].as_object()["note"]=nullptr;
        rejected=false;try{(void)formats::from_json(bad);}catch(const failure&){rejected=true;}check(rejected,"profile null is not absent note");
        std::string proto_json;
        check(google::protobuf::util::MessageToJsonString(old,&proto_json).ok(),"official ProtoJSON writer");
        V2 from_proto_json;
        check(google::protobuf::util::JsonStringToMessage(proto_json,&from_proto_json).ok()&&!from_proto_json.records(0).has_note(),"ProtoJSON loses unknown fields");
        auto official=formats::json::parse(proto_json);
        check(official.as_object().at("records").as_array()[0].as_object().at("modifiedAtMs").is_string(),"ProtoJSON int64 mapping differs from custom JSON");
        c05::Manifest copied;
        {
            google::protobuf::Arena arena;
            auto* message=google::protobuf::Arena::Create<V2>(&arena);message->CopyFrom(modern);
            check(message->GetArena()==&arena&&arena.SpaceAllocated()>0,"real Arena ownership");
            copied=formats::from_wire(*message);
        }
        check(copied==model,"owning domain copy survives Arena destruction");
        auto binary=c05::encode_manifest(model,c05::WireVersion::v2);
        check(binary&&c05::decode_manifest(*binary).value()==model,"fixed C05 format retained as baseline");
        std::cout<<"Protobuf, schema compatibility, JSON, presence and Arena checks passed\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
