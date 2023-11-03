
#include <iostream>
#include <fstream>
#include <map>
#include <cstring>
#include <vector>
#include <filesystem>

const uint32_t MAX_DUMP = 0x20;

bool g_verbose = false;

struct tag {
  uint16_t name;
  uint16_t type;
  uint32_t size;
  uint32_t value;
  uint32_t offset;
};

void print_address(std::basic_istream<char>& in)
{
  const uint32_t pos = static_cast<uint32_t>(in.tellg());
  printf("%08x: ", pos);
}

template<typename T>
T read(std::basic_istream<char>& in)
{
  T v;
  in.read(reinterpret_cast<char*>(&v), sizeof(T));
  return v;
}

template<typename T>
T trace(std::basic_istream<char>& in);

template<>
uint32_t trace(std::basic_istream<char>& in)
{
  if (g_verbose) { print_address(in); }
  const uint32_t v = read<uint32_t>(in);
  if (g_verbose) { printf("%08x\n", v); }
  return v;
}

template<>
uint16_t trace(std::basic_istream<char>& in)
{
  if (g_verbose) { print_address(in); }
  const uint32_t v = read<uint16_t>(in);
  if (g_verbose) { printf("%04x\n", 0xffff & v); }
  return v;
}

void dump(std::basic_istream<char>& in, uint32_t size, uint32_t offset)
{
  char str[MAX_DUMP];
  const auto pos = in.tellg();
  in.seekg(offset);
  for (uint32_t i = 0; i < MAX_DUMP; i++) {
    if (i < size) {
      str[i] = read<uint8_t>(in);
      printf("%02x ", 0xff & str[i]);
    } else {
      printf("   ");
    }
  }
  printf(size > MAX_DUMP ? "... | " : "    | ");
  for (uint32_t i = 0; i < MAX_DUMP; i++) {
    if (i >= size) {
      break;
    }
    if (str[i] >= 0x20 && str[i] <= 0x7f) {
      printf("%c", str[i]);
    } else {
      printf(" ");
    }
  }
  if (size > MAX_DUMP) {
    printf("...");
  }
  in.seekg(pos);
}

std::map<uint16_t, tag> trace_tags(std::basic_istream<char>& in)
{
  std::map<uint16_t, tag> tags;
  const uint16_t length = trace<uint16_t>(in);
  for (auto i = 0; i < length; i++) {
    if (g_verbose) { print_address(in); }
    const uint32_t offset = static_cast<uint32_t>(in.tellg());
    const uint16_t name = read<uint16_t>(in);
    const uint16_t type = read<uint16_t>(in);
    const uint32_t size = read<uint32_t>(in);
    const uint32_t value = read<uint32_t>(in);
    if (g_verbose) {
      printf("%04x %04x %08x %08x |", 0xffff & name, 0xffff & type, size, value);
      if (type == 5) {
        dump(in, 8, value);
      } else if (size > 4) {
        dump(in, size, value);
      } else {
        dump(in, 0, 0);
      }
      printf("\n");
    }
    tags.insert(std::pair<uint16_t, tag>(name, tag({ name, type, size, value, offset })));
  }
  return tags;
}

uint32_t trace_versionstack_offset(std::basic_istream<char>& in)
{
  const auto pos = in.tellg();
  in.seekg(0);

  trace<uint32_t>(in);
  trace<uint32_t>(in);

  auto tags0 = trace_tags(in);

  const uint32_t addr = trace<uint32_t>(in);

  in.seekg(addr);
  auto tags1 = trace_tags(in);

  in.seekg(tags0[0xc634].value);
  auto tags2 = trace_tags(in);

  in.seekg(pos);

  return tags2[0x7241].offset + 8;
}

void print_usage()
{
  std::cout << "usage:" << std::endl;
  std::cout << "    arwtool trace RAW-FILE" << std::endl;
  std::cout << "    arwtool remove-versionstack RAW-FILE" << std::endl;
  std::cout << "    arwtool store-versionstack RAW-FILE VERSIONSTACK-FILE" << std::endl;
  std::cout << "    arwtool restore-versionstack RAW-FILE VERSIONSTACK-FILE" << std::endl;
}

int main(int argc, char** argv)
{
  if (argc < 3) {
    print_usage();
    return 0;
  }
  if (argc == 3 && 0 == strcmp(argv[1], "trace")) {
    std::ifstream in(argv[2], std::ios::binary | std::ios::in);
    if (!in) {
      std::cerr << "Failed to open " << argv[2] << std::endl;
      return -1;
    }
    g_verbose = true;
    trace_versionstack_offset(in);
    return 0;
  }
  if (argc == 3 && 0 == strcmp(argv[1], "remove-versionstack")) {
    std::fstream raw(argv[2], std::ios::binary | std::ios::in | std::ios::out);
    if (!raw) {
      std::cerr << "Failed to open " << argv[2] << std::endl;
      return -1;
    }
    const uint32_t zero = 0;
    const uint32_t offset = trace_versionstack_offset(raw);
    raw.seekg(offset);
    const uint32_t vs_start = read<uint32_t>(raw);
    if (!vs_start) {
      std::cerr << "This file doesn't have version stack." << std::endl;
      return -1;
    }
    if (vs_start < offset + 4) {
      std::cerr << "Failed to parse this file." << std::endl;
      return -1;
    }
    raw.seekp(offset);
    raw.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
    raw.close();
    std::filesystem::resize_file(argv[2], vs_start);
    return 0;
  }
  if (argc == 4 && 0 == strcmp(argv[1], "store-versionstack")) {
    std::ifstream raw(argv[2], std::ios::binary | std::ios::in);
    if (!raw) {
      std::cerr << "Failed to open " << argv[2] << std::endl;
      return -1;
    }
    std::ofstream storage(argv[3], std::ios::binary | std::ios::out);
    if (!storage) {
      std::cerr << "Failed to open " << argv[3] << std::endl;
      return -1;
    }
    const uint32_t zero = 0;
    const uint32_t offset = trace_versionstack_offset(raw);
    raw.seekg(offset);
    const uint32_t vs_start = read<uint32_t>(raw);
    if (!vs_start) {
      std::cerr << "This file doesn't have version stack." << std::endl;
      return -1;
    }
    if (vs_start < offset + 4) {
      std::cerr << "Failed to parse this file." << std::endl;
      return -1;
    }
    raw.seekg(0, std::ios::end);
    const uint32_t vs_end = raw.tellg();
    if (vs_start >= vs_end) {
      std::cerr << "Failed to parse this file." << std::endl;
      return -1;
    }
    const uint32_t vs_size = vs_end - vs_start;
    std::vector<char> buf(vs_size);
    raw.seekg(vs_start);
    raw.read(buf.data(), vs_size);
    storage.write(buf.data(), vs_size);
    return 0;
  }
  if (argc == 4 && 0 == strcmp(argv[1], "restore-versionstack")) {
    std::fstream raw(argv[2], std::ios::binary | std::ios::in | std::ios::out);
    if (!raw) {
      std::cerr << "Failed to open " << argv[2] << std::endl;
      return -1;
    }
    std::ifstream storage(argv[3], std::ios::binary | std::ios::in);
    if (!storage) {
      std::cerr << "Failed to open " << argv[3] << std::endl;
      return -1;
    }
    const uint32_t zero = 0;
    const uint32_t offset = trace_versionstack_offset(raw);
    raw.seekg(offset);
    const uint32_t vs_start = read<uint32_t>(raw);
    if (!vs_start) {
      raw.seekg(0, std::ios::end);
      const uint32_t filesize = raw.tellg();
      raw.seekp(offset);
      raw.write(reinterpret_cast<const char*>(&filesize), sizeof(filesize));
      raw.seekp(0, std::ios::end);
    } else if (vs_start >= offset + 4) {
      raw.seekp(vs_start);
    } else {
      std::cerr << "Failed to parse this file." << std::endl;
      return -1;
    }
    storage.seekg(0, std::ios::end);
    const uint32_t vs_size = storage.tellg();
    std::vector<char> buf(vs_size);
    storage.seekg(0);
    storage.read(buf.data(), vs_size);
    raw.write(buf.data(), vs_size);
    const uint32_t newsize = raw.tellp();
    raw.close();
    std::filesystem::resize_file(argv[2], newsize);
    return 0;
  }
  return 0;
}
