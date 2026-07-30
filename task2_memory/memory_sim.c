// ST5004CEM - Task 2: Memory Management Simulation
// memory_sim.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_FRAMES      4
#define PAGE_SIZE_KB    4
#define REF_STRING_LEN  20

typedef struct {
    int page;
    int loaded_at;
    int last_used;
} frame_t;

typedef struct {
    int faults;
    int hits;
    int total_refs;
} stats_t;

// ---------- Frame Helpers ----------

static int find_page(frame_t frames[], int n, int page) {
    for (int i = 0; i < n; i++)
        if (frames[i].page == page) return i;
    return -1;
}

static int find_empty(frame_t frames[], int n) {
    for (int i = 0; i < n; i++)
        if (frames[i].page == -1) return i;
    return -1;
}

// ---------- FIFO Victim Selection ----------

static int find_victim_fifo(frame_t frames[], int n) {
    int victim = 0;
    for (int i = 1; i < n; i++)
        if (frames[i].loaded_at < frames[victim].loaded_at) victim = i;
    return victim;
}

// ---------- LRU Victim Selection ----------

static int find_victim_lru(frame_t frames[], int n) {
    int victim = 0;
    for (int i = 1; i < n; i++)
        if (frames[i].last_used < frames[victim].last_used) victim = i;
    return victim;
}

static void print_frames(frame_t frames[], int n) {
    printf("   frames: [");
    for (int i = 0; i < n; i++) {
        if (frames[i].page == -1) printf(" _ ");
        else printf(" %d ", frames[i].page);
    }
    printf("]\n");
}

// ---------- Simulation Runner (FIFO / LRU) ----------

static void run_simulation(int ref_string[], int len, int algorithm, stats_t *out) {
    frame_t frames[NUM_FRAMES];
    for (int i = 0; i < NUM_FRAMES; i++) {
        frames[i].page = -1;
        frames[i].loaded_at = -1;
        frames[i].last_used = -1;
    }

    stats_t s = {0, 0, 0};
    const char *name = algorithm == 0 ? "FIFO" : "LRU";
    printf("\n--- %s simulation (page size = %dKB, frames = %d) ---\n",
           name, PAGE_SIZE_KB, NUM_FRAMES);

    for (int t = 0; t < len; t++) {
        int page = ref_string[t];
        s.total_refs++;
        int idx = find_page(frames, NUM_FRAMES, page);

        if (idx != -1) {
            s.hits++;
            frames[idx].last_used = t;
            printf("t=%2d ref=%2d : HIT ", t, page);
        } else {
            s.faults++;
            int slot = find_empty(frames, NUM_FRAMES);
            if (slot == -1) {
                slot = (algorithm == 0) ? find_victim_fifo(frames, NUM_FRAMES)
                                         : find_victim_lru(frames, NUM_FRAMES);
                printf("t=%2d ref=%2d : FAULT (evict page %d) ", t, page, frames[slot].page);
            } else {
                printf("t=%2d ref=%2d : FAULT (load into free frame) ", t, page);
            }
            frames[slot].page = page;
            frames[slot].loaded_at = t;
            frames[slot].last_used = t;
        }
        print_frames(frames, NUM_FRAMES);
    }

    printf("\n%s summary: refs=%d, faults=%d, hits=%d, "
           "fault_ratio=%.2f%%, hit_ratio=%.2f%%\n",
           name, s.total_refs, s.faults, s.hits,
           100.0 * s.faults / s.total_refs, 100.0 * s.hits / s.total_refs);

    *out = s;
}

// ---------- Main ----------

int main(void) {
    int ref_string[REF_STRING_LEN] = {
        1, 2, 3, 4, 1, 2, 5, 1, 2, 3,
        4, 5, 1, 2, 3, 4, 5, 6, 1, 2
    };

    printf("Reference string (%d accesses): ", REF_STRING_LEN);
    for (int i = 0; i < REF_STRING_LEN; i++) printf("%d ", ref_string[i]);
    printf("\n");

    stats_t fifo_stats, lru_stats;
    run_simulation(ref_string, REF_STRING_LEN, 0, &fifo_stats);
    run_simulation(ref_string, REF_STRING_LEN, 1, &lru_stats);

    printf("\n=========================================================\n");
    printf(" Comparison\n");
    printf("=========================================================\n");
    printf("Algorithm  Faults  Hits  Fault%%   Hit%%\n");
    printf("FIFO         %2d     %2d   %5.2f  %5.2f\n",
           fifo_stats.faults, fifo_stats.hits,
           100.0 * fifo_stats.faults / fifo_stats.total_refs,
           100.0 * fifo_stats.hits / fifo_stats.total_refs);
    printf("LRU          %2d     %2d   %5.2f  %5.2f\n",
           lru_stats.faults, lru_stats.hits,
           100.0 * lru_stats.faults / lru_stats.total_refs,
           100.0 * lru_stats.hits / lru_stats.total_refs);

    return 0;
}
