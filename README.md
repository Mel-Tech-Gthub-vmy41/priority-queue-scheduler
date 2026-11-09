# ⚙️ Priority Queue Task Scheduler in C

A robot task scheduler built on a **min-heap priority queue** implemented from scratch in C. Simulates a warehouse robot managing tasks with different urgency levels.

## 🧠 Data Structure: Min-Heap

| Operation    | Time Complexity |
|-------------|----------------|
| Insert       | O(log n)        |
| Extract-min  | O(log n)        |
| Peek minimum | O(1)            |
| Build heap   | O(n)            |

The heap guarantees the **highest-priority task always executes first**, regardless of insertion order.

## 🤖 Robot Tasks & Priorities

| Priority | Level    | Tasks                        |
|----------|----------|------------------------------|
| 1        | CRITICAL | Charge Battery, Emergency Stop |
| 2        | HIGH     | Pick Item                    |
| 3        | MEDIUM   | Move to Shelf, Scan Barcode  |
| 4        | LOW      | Report Status                |
| 5        | IDLE     | Standby                      |

## 🚀 Build & Run

```bash
# Compile
make

# Run
./scheduler

# Or one command
make run
```

## 🎮 Demo Scenarios

1. **Warehouse Morning Shift** — normal task scheduling, watch priority ordering
2. **Emergency Interrupt** — inject critical tasks mid-queue, they jump to front instantly
3. **Heap Structure Analysis** — visualise the internal heap array and extraction order

## 💡 Key Concepts Demonstrated

- **Min-heap** built from scratch (no stdlib)
- **Sift-up** after insert, **sift-down** after extract
- **FIFO tie-breaking** when priorities are equal (sequence number)
- **Task preemption** — emergency tasks inserted at any time jump to front
- **Robot state machine** — battery, location, item tracking

## 📁 Files

```
priority_scheduler/
├── scheduler.c   ← full implementation (~350 lines)
├── Makefile
└── README.md
```
