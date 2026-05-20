//
// Created by rvova on 26.04.2026.
//

#ifndef MINIDB_CONSTANTS_H
#define MINIDB_CONSTANTS_H

namespace db {
    inline constexpr size_t PAGE_SIZE = 4096;
}

namespace entrypoint {
    inline constexpr int MIN_COUNT_STORAGE_NODES = 1;
    inline constexpr int MAX_COUNT_STORAGE_NODES = 8;

    inline constexpr int COUNT_THREADS = 2;
    inline constexpr int SERVER_PORT = 1235;
    inline constexpr int HEARTBEAT_PORT = 1236;
}

#endif //MINIDB_CONSTANTS_H
