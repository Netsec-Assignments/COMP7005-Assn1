#include <cstdint>

enum op_type : uint32_t {
    GET,
    SEND
};

struct request_packet {
    op_type op;
    uint32_t request_size;
};
