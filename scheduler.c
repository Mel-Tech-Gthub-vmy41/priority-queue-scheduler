/*
 * ============================================================
 *  Priority Queue Task Scheduler  —  Robot Simulation
 *  Data structure: Min-Heap (lower priority number = runs first)
 *
 *  Tasks:
 *    CHARGE_BATTERY   priority 1  (critical)
 *    EMERGENCY_STOP   priority 1  (critical)
 *    PICK_ITEM        priority 2  (high)
 *    MOVE_TO_SHELF    priority 3  (medium)
 *    SCAN_BARCODE     priority 3  (medium)
 *    REPORT_STATUS    priority 4  (low)
 *    IDLE             priority 5  (lowest)
 * ============================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>   /* usleep, sleep */

/* ── ANSI colour codes ───────────────────────────────────────── */
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BRED    "\033[1;31m"
#define BGREEN  "\033[1;32m"
#define BYELLOW "\033[1;33m"
#define BCYAN   "\033[1;36m"
#define BWHITE  "\033[1;37m"

/* ── Constants ───────────────────────────────────────────────── */
#define MAX_TASKS      64
#define MAX_NAME_LEN   40
#define MAX_DESC_LEN   80

/* ── Task types ──────────────────────────────────────────────── */
typedef enum {
    CHARGE_BATTERY,
    EMERGENCY_STOP,
    PICK_ITEM,
    MOVE_TO_SHELF,
    SCAN_BARCODE,
    REPORT_STATUS,
    IDLE
} TaskType;

/* ── Task structure ──────────────────────────────────────────── */
typedef struct {
    int       id;
    TaskType  type;
    int       priority;      /* 1 = highest, 5 = lowest */
    char      name[MAX_NAME_LEN];
    char      description[MAX_DESC_LEN];
    int       duration_ms;   /* simulated execution time */
    time_t    enqueue_time;
    int       sequence;      /* tie-break: earlier enqueue wins */
} Task;

/* ── Min-Heap ────────────────────────────────────────────────── */
typedef struct {
    Task  data[MAX_TASKS];
    int   size;
    int   total_enqueued;
    int   total_executed;
} MinHeap;

/* ── Robot state ─────────────────────────────────────────────── */
typedef struct {
    int   battery;        /* 0–100 */
    int   items_picked;
    int   shelves_visited;
    int   tasks_done;
    char  current_location[32];
    int   is_charging;
} RobotState;

/* ================================================================
 * Min-Heap operations
 * ================================================================ */

/* Compare: returns 1 if a should come before b */
int task_has_priority(const Task *a, const Task *b) {
    if (a->priority != b->priority)
        return a->priority < b->priority;
    return a->sequence < b->sequence;   /* FIFO tie-break */
}

void swap_tasks(Task *a, Task *b) {
    Task tmp = *a; *a = *b; *b = tmp;
}

/* Bubble a node up to restore heap property after insert */
void heap_sift_up(MinHeap *h, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (task_has_priority(&h->data[i], &h->data[parent])) {
            swap_tasks(&h->data[i], &h->data[parent]);
            i = parent;
        } else break;
    }
}

/* Bubble a node down to restore heap property after extract */
void heap_sift_down(MinHeap *h, int i) {
    while (1) {
        int left  = 2*i + 1;
        int right = 2*i + 2;
        int best  = i;

        if (left  < h->size && task_has_priority(&h->data[left],  &h->data[best]))
            best = left;
        if (right < h->size && task_has_priority(&h->data[right], &h->data[best]))
            best = right;

        if (best == i) break;
        swap_tasks(&h->data[i], &h->data[best]);
        i = best;
    }
}

/* Insert a task — O(log n) */
int heap_push(MinHeap *h, Task t) {
    if (h->size >= MAX_TASKS) {
        printf(RED "  [HEAP] Queue full! Cannot add task.\n" RESET);
        return 0;
    }
    t.sequence        = h->total_enqueued++;
    t.enqueue_time    = time(NULL);
    h->data[h->size]  = t;
    heap_sift_up(h, h->size);
    h->size++;
    return 1;
}

/* Extract minimum — O(log n) */
Task heap_pop(MinHeap *h) {
    Task top = h->data[0];
    h->data[0] = h->data[--h->size];
    heap_sift_down(h, 0);
    h->total_executed++;
    return top;
}

/* Peek at minimum without removing — O(1) */
Task *heap_peek(MinHeap *h) {
    return h->size > 0 ? &h->data[0] : NULL;
}

void heap_init(MinHeap *h) {
    h->size           = 0;
    h->total_enqueued = 0;
    h->total_executed = 0;
}

/* ================================================================
 * Task factory helpers
 * ================================================================ */
static int next_id = 1;

Task make_task(TaskType type, const char *name,
               const char *desc, int priority, int duration_ms) {
    Task t;
    memset(&t, 0, sizeof(t));
    t.id          = next_id++;
    t.type        = type;
    t.priority    = priority;
    t.duration_ms = duration_ms;
    strncpy(t.name,        name, MAX_NAME_LEN - 1);
    strncpy(t.description, desc, MAX_DESC_LEN - 1);
    return t;
}

/* Convenience constructors */
Task task_charge(int pct) {
    char desc[MAX_DESC_LEN];
    snprintf(desc, sizeof(desc), "Battery at %d%% — recharge now", pct);
    return make_task(CHARGE_BATTERY, "CHARGE BATTERY", desc, 1, 3000);
}
Task task_emergency(const char *reason) {
    char desc[MAX_DESC_LEN];
    snprintf(desc, sizeof(desc), "Emergency: %s", reason);
    return make_task(EMERGENCY_STOP, "EMERGENCY STOP", desc, 1, 500);
}
Task task_pick(const char *item) {
    char desc[MAX_DESC_LEN];
    snprintf(desc, sizeof(desc), "Pick up: %s", item);
    return make_task(PICK_ITEM, "PICK ITEM", desc, 2, 1500);
}
Task task_move(const char *shelf) {
    char desc[MAX_DESC_LEN];
    snprintf(desc, sizeof(desc), "Navigate to shelf %s", shelf);
    return make_task(MOVE_TO_SHELF, "MOVE TO SHELF", desc, 3, 2000);
}
Task task_scan(const char *barcode) {
    char desc[MAX_DESC_LEN];
    snprintf(desc, sizeof(desc), "Scan barcode: %s", barcode);
    return make_task(SCAN_BARCODE, "SCAN BARCODE", desc, 3, 800);
}
Task task_report(void) {
    return make_task(REPORT_STATUS, "REPORT STATUS",
                     "Send telemetry to base station", 4, 600);
}
Task task_idle(void) {
    return make_task(IDLE, "IDLE", "No tasks — entering standby", 5, 1000);
}

/* ================================================================
 * Display helpers
 * ================================================================ */
const char *priority_label(int p) {
    switch (p) {
        case 1: return BRED   "CRITICAL" RESET;
        case 2: return BYELLOW "HIGH    " RESET;
        case 3: return BCYAN  "MEDIUM  " RESET;
        case 4: return BLUE   "LOW     " RESET;
        default: return WHITE "IDLE    " RESET;
    }
}

const char *task_icon(TaskType t) {
    switch (t) {
        case CHARGE_BATTERY: return "⚡";
        case EMERGENCY_STOP: return "🚨";
        case PICK_ITEM:      return "📦";
        case MOVE_TO_SHELF:  return "🚗";
        case SCAN_BARCODE:   return "📷";
        case REPORT_STATUS:  return "📡";
        default:             return "💤";
    }
}

void print_separator(void) {
    printf(BLUE "  ════════════════════════════════════════════════════\n" RESET);
}

void print_header(void) {
    printf("\n");
    printf(BCYAN "  ╔══════════════════════════════════════════════════╗\n");
    printf(       "  ║     🤖  ROBOT PRIORITY QUEUE SCHEDULER          ║\n");
    printf(       "  ║        Min-Heap  |  C Implementation             ║\n");
    printf(       "  ╚══════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
}

void print_robot_state(const RobotState *r) {
    const char *bat_col = r->battery > 50 ? BGREEN :
                          r->battery > 20 ? BYELLOW : BRED;
    printf(BOLD "  Robot State:\n" RESET);
    printf("    Battery    : %s%d%%\n" RESET,   bat_col, r->battery);
    printf("    Location   : " CYAN "%s\n" RESET, r->current_location);
    printf("    Items picked: " GREEN "%d\n" RESET, r->items_picked);
    printf("    Tasks done  : " GREEN "%d\n" RESET, r->tasks_done);
    printf("    Charging    : %s\n", r->is_charging ? BGREEN "YES" RESET
                                                    : RED "NO" RESET);
}

void print_queue(const MinHeap *h) {
    if (h->size == 0) {
        printf("  " YELLOW "(queue is empty)\n" RESET);
        return;
    }
    printf(BOLD "  %-4s %-18s %-10s %s\n" RESET,
           "ID", "Task", "Priority", "Description");
    print_separator();
    /* Note: heap array is not sorted except root is min.
       We display in array order to show heap structure. */
    for (int i = 0; i < h->size; i++) {
        const Task *t = &h->data[i];
        printf("  #%-3d %-18s [%s] %s\n",
               t->id, t->name,
               priority_label(t->priority),
               t->description);
    }
}

void animate_task(const Task *t, RobotState *r) {
    printf("\n");
    print_separator();
    printf("  %s Executing: " BWHITE "%s\n" RESET, task_icon(t->type), t->name);
    printf("  " YELLOW "Description : %s\n" RESET, t->description);
    printf("  " CYAN   "Priority    : %s  |  Est. time: %d ms\n" RESET,
           priority_label(t->priority), t->duration_ms);
    print_separator();

    /* Simulate work with a progress bar */
    int steps = 20;
    printf("  Progress: [");
    fflush(stdout);
    for (int i = 0; i < steps; i++) {
        struct timespec ts = {0, (long)(t->duration_ms) * 1000000L / steps};
        nanosleep(&ts, NULL);
        printf(GREEN "█" RESET);
        fflush(stdout);
    }
    printf("] " BGREEN "DONE\n" RESET);

    /* Update robot state */
    r->tasks_done++;
    switch (t->type) {
        case CHARGE_BATTERY:
            r->battery     = 100;
            r->is_charging = 0;
            strcpy(r->current_location, "Charging Dock");
            break;
        case EMERGENCY_STOP:
            strcpy(r->current_location, "Safe Zone");
            break;
        case PICK_ITEM:
            r->items_picked++;
            r->battery -= 5;
            break;
        case MOVE_TO_SHELF:
            r->shelves_visited++;
            r->battery -= 8;
            /* Extract shelf name from description */
            sscanf(t->description, "Navigate to shelf %31s",
                   r->current_location);
            break;
        case SCAN_BARCODE:
            r->battery -= 2;
            break;
        case REPORT_STATUS:
            r->battery -= 1;
            break;
        case IDLE:
            break;
    }

    /* Auto-trigger charge warning */
    if (r->battery <= 20 && !r->is_charging) {
        printf("  " BRED "⚠  Battery critical (%d%%)! Charge task will be prioritised.\n" RESET,
               r->battery);
        r->is_charging = 1;
    }
    printf("\n");
}

/* ================================================================
 * Demo scenarios
 * ================================================================ */
void demo_warehouse_shift(MinHeap *h, RobotState *r) {
    printf(BWHITE "\n  ── Scenario: Warehouse Morning Shift ──\n\n" RESET);

    /* Add tasks in arbitrary order — heap will sort them */
    heap_push(h, task_report());
    heap_push(h, task_move("B-12"));
    heap_push(h, task_pick("Widget A"));
    heap_push(h, task_scan("8901234567890"));
    heap_push(h, task_move("C-07"));
    heap_push(h, task_pick("Gadget X"));
    heap_push(h, task_scan("7612345678901"));
    heap_push(h, task_report());

    printf("  Tasks added to queue (%d total):\n", h->size);
    print_queue(h);
    printf("\n  " YELLOW "Watch how they execute in PRIORITY order...\n\n" RESET);
    sleep(1);

    while (h->size > 0) {
        Task t = heap_pop(h);
        animate_task(&t, r);
        printf("  Remaining in queue: %d task(s)\n", h->size);
        print_robot_state(r);
        printf("\n");
        sleep(1);
    }
}

void demo_emergency_interrupt(MinHeap *h, RobotState *r) {
    printf(BWHITE "\n  ── Scenario: Emergency Mid-Shift ──\n\n" RESET);

    /* Fill queue with normal work */
    heap_push(h, task_move("A-01"));
    heap_push(h, task_pick("Package Y"));
    heap_push(h, task_scan("1234567890123"));
    heap_push(h, task_report());

    printf("  Normal tasks queued (%d). Now injecting EMERGENCY...\n\n", h->size);
    sleep(1);

    /* Emergency arrives mid-queue — it jumps to front */
    heap_push(h, task_emergency("Obstacle detected on path"));
    heap_push(h, task_charge(r->battery));

    printf("  Queue after emergency injection:\n");
    print_queue(h);
    printf("\n  " BRED "Critical tasks will execute FIRST regardless of insertion order.\n\n" RESET);
    sleep(1);

    while (h->size > 0) {
        Task t = heap_pop(h);
        animate_task(&t, r);
        printf("  Remaining in queue: %d task(s)\n", h->size);
        sleep(1);
    }
}

void demo_heap_analysis(void) {
    printf(BWHITE "\n  ── Heap Structure Analysis ──\n\n" RESET);
    MinHeap h;
    heap_init(&h);

    int pris[] = {4, 2, 5, 1, 3, 2, 1};
    char *names[] = {"Report","Pick","Idle","Charge","Move","Pick2","Emergency"};
    for (int i = 0; i < 7; i++) {
        Task t;
        memset(&t, 0, sizeof(t));
        t.id       = i+1;
        t.priority = pris[i];
        strncpy(t.name, names[i], MAX_NAME_LEN-1);
        snprintf(t.description, MAX_DESC_LEN, "Demo task %d", i+1);
        heap_push(&h, t);
    }

    printf("  Inserted 7 tasks with priorities: 4,2,5,1,3,2,1\n\n");
    printf("  Heap array (index 0 = root = minimum):\n");
    printf(BOLD "  Idx  ID   Priority  Name\n" RESET);
    print_separator();
    for (int i = 0; i < h.size; i++) {
        printf("  [%d]  #%-3d  P%d        %s\n",
               i, h.data[i].id, h.data[i].priority, h.data[i].name);
    }

    printf("\n  Extracting all in priority order:\n");
    print_separator();
    int order = 1;
    while (h.size > 0) {
        Task t = heap_pop(&h);
        printf("  %d. P%d  %s\n", order++, t.priority, t.name);
    }

    printf("\n  " BGREEN "✓ Min-heap always extracts the lowest priority number first.\n" RESET);
    printf("  " CYAN   "  Insert: O(log n)  |  Extract-min: O(log n)  |  Peek: O(1)\n\n" RESET);
}

/* ================================================================
 * Main menu
 * ================================================================ */
void print_menu(void) {
    printf(BOLD "\n  Choose a demo:\n\n" RESET);
    printf("  " BGREEN "[1]" RESET " Warehouse Morning Shift  — normal task scheduling\n");
    printf("  " BRED   "[2]" RESET " Emergency Interrupt      — high-priority preemption\n");
    printf("  " BCYAN  "[3]" RESET " Heap Structure Analysis  — visualise the min-heap\n");
    printf("  " BYELLOW"[4]" RESET " Run All Demos\n");
    printf("  " WHITE  "[0]" RESET " Exit\n\n");
    printf("  Enter choice: ");
}

int main(void) {
    srand((unsigned)time(NULL));
    print_header();

    int choice;
    do {
        print_menu();
        if (scanf("%d", &choice) != 1) break;

        MinHeap    h; heap_init(&h);
        RobotState r;
        memset(&r, 0, sizeof(r));
        r.battery = 85;
        strcpy(r.current_location, "Home Base");

        printf("\n");
        switch (choice) {
            case 1:
                demo_warehouse_shift(&h, &r);
                break;
            case 2:
                /* Start with lower battery to show charge trigger */
                r.battery = 25;
                demo_emergency_interrupt(&h, &r);
                break;
            case 3:
                demo_heap_analysis();
                break;
            case 4:
                demo_warehouse_shift(&h, &r);
                memset(&r, 0, sizeof(r));
                r.battery = 25;
                strcpy(r.current_location, "Home Base");
                heap_init(&h);
                demo_emergency_interrupt(&h, &r);
                demo_heap_analysis();
                break;
            case 0:
                printf(BGREEN "  Scheduler shutdown. Goodbye!\n\n" RESET);
                break;
            default:
                printf(RED "  Invalid choice.\n" RESET);
        }
    } while (choice != 0);

    return 0;
}
