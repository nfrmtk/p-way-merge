#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
namespace pmerge::fs {
struct Record {
  std::uint64_t hash;
  std::uint64_t key[3];
  std::uint64_t stub[3];
  std::uint64_t value;
};
class RecordsSource {
  explicit RecordsSource(std::ifstream source_);

 public:
  explicit RecordsSource(const std::filesystem::path& path);
  RecordsSource Clone();
  Record GetRecord();

  [[nodiscard]] bool HasRecords() const;

 private:
  std::ifstream source_;
};
}  // namespace pmerge::fs