#include "data_tamer/types.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <unordered_map>

namespace DataTamer {

static const std::array<std::string, TypesCount> kNames = {
    "bool", "char",
    "int8", "uint8",
    "int16", "uint16",
    "int32", "uint32",
    "int64", "uint64",
    "float32", "float64",
    "other"
};

const std::string& ToStr(const BasicType& type)
{
  return kNames[static_cast<size_t>(type)];
}

BasicType FromStr(const std::string& str)
{
  static const auto kMap = []() {
    std::unordered_map<std::string, BasicType> map;
    for(size_t i=0; i<TypesCount; i++)
    {
      auto type = static_cast<BasicType>(i);
      map[ToStr(type)] = type;
    }
    return map;
  }();

  auto const it = kMap.find(str);
  return it == kMap.end() ? BasicType::OTHER : it->second;
}

size_t SizeOf(const BasicType& type)
{
  static constexpr std::array<size_t, TypesCount> kSizes =
      { 1, 1,
        1, 1,
        2, 2, 4, 4, 8, 8,
        4, 8, 0 };
  return kSizes[static_cast<size_t>(type)];
}

template<typename T>
T DeserializeImpl(const void *data)
{
  T var;
  std::memcpy(&var, data, sizeof(T));
  return var;
}

VarNumber DeserializeAsVarType(const BasicType &type, const void *data)
{
  switch(type)
  {
    case BasicType::BOOL: return DeserializeImpl<bool>(data);
    case BasicType::CHAR: return DeserializeImpl<char>(data);

    case BasicType::INT8: return DeserializeImpl<int8_t>(data);
    case BasicType::UINT8: return DeserializeImpl<uint8_t>(data);

    case BasicType::INT16: return DeserializeImpl<int16_t>(data);
    case BasicType::UINT16: return DeserializeImpl<uint16_t>(data);

    case BasicType::INT32: return DeserializeImpl<int32_t>(data);
    case BasicType::UINT32: return DeserializeImpl<uint32_t>(data);

    case BasicType::INT64: return DeserializeImpl<int64_t>(data);
    case BasicType::UINT64: return DeserializeImpl<uint64_t>(data);

    case BasicType::FLOAT32: return DeserializeImpl<float>(data);
    case BasicType::FLOAT64: return DeserializeImpl<double>(data);

    case BasicType::OTHER:
      return double(std::numeric_limits<double>::quiet_NaN());
  }
  return {};
}

uint64_t AddFieldToHash(const Schema::Field &field, uint64_t hash)
{
  // https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
  const std::hash<std::string> str_hasher;
  const std::hash<BasicType> type_hasher;
  const std::hash<bool> bool_hasher;
  const std::hash<uint16_t> uint_hasher;

  auto combine = [&hash](const auto& hasher, const auto& val)
  {
    hash ^= hasher(val) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  };

  combine(str_hasher, field.name);
  combine(type_hasher, field.type);
  if(field.type == BasicType::OTHER && field.custom_type)
  {
    combine(str_hasher, field.custom_type->typeName());
  }
  combine(bool_hasher, field.is_vector);
  combine(uint_hasher, field.array_size);
  return hash;
}

std::ostream& operator<<(std::ostream &os, const Schema::Field &field)
{
  if(field.type == BasicType::OTHER && field.custom_type)
  {
    os << field.custom_type->typeName();
  }
  else {
    os << ToStr(field.type);
  }

  if(field.is_vector && field.array_size != 0)
  {
    os <<"[" << field.array_size << "]";
  }
  if(field.is_vector && field.array_size == 0)
  {
    os <<"[]";
  }
  os << ' ' << field.name;
  return os;
}

std::ostream& operator<<(std::ostream &os, const Schema &schema)
{
  os << "__version__: " << SCHEMA_VERSION << "\n";
  os << "__hash__: " << schema.hash << "\n";
  os << "__channel_name__: " << schema.channel_name << "\n";

  std::map<std::string, std::shared_ptr<CustomTypeInfo>> custom_types;
  for(const auto& field: schema.fields)
  {
    if(field.custom_type)
    {
      custom_types[field.custom_type->typeName()] = field.custom_type;
    }
    os << field << "\n";
  }
  for(const auto& [name, type]: custom_types)
  {
    const char* custom_schema = type->typeSchema();
    if(custom_schema){
      os << "---------\n"
         << type->typeName() << "\n"
         << "---------\n"
         << custom_schema;
    }
  }

  return os;
}

bool Schema::Field::operator==(const Field &other) const
{
  return is_vector == other.is_vector &&
         type == other.type &&
         array_size == other.array_size &&
         name == other.name;
}

bool Schema::Field::operator!=(const Field &other) const
{
  return !(*this == other);
}


}  // namespace DataTamer
