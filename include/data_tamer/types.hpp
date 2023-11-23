#pragma once

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <vector>
#include <variant>

namespace DataTamer {

constexpr int SCHEMA_VERSION = 3;

enum class BasicType
{
  BOOL,
  CHAR,
  INT8,
  UINT8,

  INT16,
  UINT16,

  INT32,
  UINT32,

  INT64,
  UINT64,

  FLOAT32,
  FLOAT64,
  OTHER
};

constexpr size_t TypesCount = 13;

using VarNumber = std::variant<
    bool, char,
    int8_t, uint8_t,
    int16_t, uint16_t,
    int32_t, uint32_t,
    int64_t, uint64_t,
    float, double >;

/// Reverse operation of ValuePtr::serialize
[[nodiscard]] VarNumber DeserializeAsVarType(const BasicType& type, const void* data);

/// Return the number of bytes needed to serialize the type
[[nodiscard]] size_t SizeOf(const BasicType& type);

/// Return the name of the type
[[nodiscard]] const std::string &ToStr(const BasicType& type);

/// Convert string to its type
[[nodiscard]] BasicType FromStr(const std::string &str);

template <typename T>
inline constexpr BasicType GetBasicType()
{
  if constexpr (std::is_same_v<T, bool>) return BasicType::BOOL;
  if constexpr (std::is_same_v<T, char>) return BasicType::CHAR;
  if constexpr (std::is_same_v<T, std::int8_t>) return BasicType::INT8;
  if constexpr (std::is_same_v<T, std::uint8_t>) return BasicType::UINT8;

  if constexpr (std::is_same_v<T, std::int16_t>) return BasicType::INT16;
  if constexpr (std::is_same_v<T, std::uint16_t>) return BasicType::UINT16;

  if constexpr (std::is_same_v<T, std::int32_t>) return BasicType::INT32;
  if constexpr (std::is_same_v<T, std::uint32_t>) return BasicType::UINT32;

  if constexpr (std::is_same_v<T, std::uint64_t>) return BasicType::UINT64;
  if constexpr (std::is_same_v<T, std::int64_t>) return BasicType::INT64;

  if constexpr (std::is_same_v<T, float>) return BasicType::FLOAT32;
  if constexpr (std::is_same_v<T, double>) return BasicType::FLOAT64;

  return BasicType::OTHER;
}

struct RegistrationID
{
  size_t first_index = 0;
  size_t fields_count = 0;

  // sintactic sugar to be used to concatenate contiguous RegistrationID.
  // See example custom_type.cpp
  void operator+=(const RegistrationID& other) {
    fields_count += other.fields_count;
  }
};

// TODO: allow fields with unknown type, that DataTamer doesn't know how to serialize
struct CustomTypeInfo
{
  using Ptr = std::shared_ptr<CustomTypeInfo>;

  virtual ~CustomTypeInfo() = default;
  // name of the type, to be written in the schema string.
  virtual const char* typeName() = 0;
  // optional custom schema of the type
  virtual const char* typeSchema() { return nullptr; }
  // size in bytes of the serialized object.
  // Needed to pre-allocate memory in the buffer
  virtual uint32_t serializedSize(const void* src_instance) = 0;
  // serialize an object into a buffer. Return the size in bytes of the serialized data
  virtual uint32_t serialize(const void* src_instance, uint8_t* dst_buffer) = 0;
};

/**
 * @brief DataTamer uses a simple "flat" schema of key/value pairs (each pair is a "field").
 */
struct Schema
{
  struct Field
  {
    std::string name;
    BasicType type;
    bool is_vector = 0;
    uint16_t array_size = 0;
    std::shared_ptr<CustomTypeInfo> custom_type;

    bool operator==(const Field& other) const;
    bool operator!=(const Field& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Field& field);
  };
  std::vector<Field> fields;
  uint64_t hash = 0;
  std::string channel_name;

  friend std::ostream& operator<<(std::ostream& os, const Schema& schema);
};


[[nodiscard]]  uint64_t AddFieldToHash(const Schema::Field& field, uint64_t hash);

}  // namespace DataTamer
