#include <cstring>
#include <pmerge/fs/records_source.hpp>
namespace pmerge::fs {
Record RecordsSource::GetRecord() {
  Record record{};
  char buf[sizeof(Record)];
  source_.read(buf, sizeof(Record));
  std::memcpy(&record, buf, sizeof(Record));
  // source_.register_callback()
  return record;
}
bool RecordsSource::HasRecords() const { return static_cast<bool>(source_); }

RecordsSource Clone() {
  // RecordsSource ret{};
}

}  // namespace pmerge::fs