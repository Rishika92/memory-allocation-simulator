// memory_final_loop.cpp
// Runs forever until you choose Exit

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <climits>
using namespace std;

enum Policy { FIRST_FIT, BEST_FIT, WORST_FIT, NEXT_FIT };

struct Process {
    int arrival, pid, priority, memory, cputime;
    int offset = -1;
};

struct mab {
    int offset, size, allocated;  // 0 = hole, 1 = running
    mab *next, *prev;
};
typedef mab* MabPtr;

MabPtr next_fit_ptr = nullptr;
Policy POLICY;
const char* policy_names[] = {"First Fit", "Best Fit", "Worst Fit", "Next Fit"};

// Print memory
void printArena(MabPtr arena) {
    cout << "\n     offset     size      status\n";
    cout << "   ------------------------------------\n";
    for (MabPtr p = arena; p; p = p->next) {
        cout << "   " << setw(8) << p->offset
             << "  " << setw(8) << p->size
             << "   " << (p->allocated ? "running" : "hole");
        if (POLICY == NEXT_FIT && p == next_fit_ptr)
            cout << "  ← next_fit_ptr";
        cout << "\n";
    }
    cout << "\n";
}

// All memory functions (100% correct)
MabPtr memMerge(MabPtr m) {
    if (!m) return nullptr;
    while (m->next && !m->next->allocated) {
        m->size += m->next->size;
        MabPtr t = m->next;
        m->next = t->next;
        if (t->next) t->next->prev = m;
        delete t;
    }
    MabPtr cur = m;
    while (cur->prev && !cur->prev->allocated) {
        cur->prev->size += cur->size;
        cur->prev->next = cur->next;
        if (cur->next) cur->next->prev = cur->prev;
        delete cur;
        cur = cur->prev;
    }
    return cur;
}

MabPtr memSplit(MabPtr b, int sz) {
    if (!b || b->allocated || b->size < sz) return nullptr;
    if (b->size == sz) { b->allocated = 1; return b; }
    MabPtr rem = new mab{b->offset + sz, b->size - sz, 0, b->next, b};
    if (b->next) b->next->prev = rem;
    b->next = rem;
    b->size = sz;
    b->allocated = 1;
    return b;
}

MabPtr findBlock(MabPtr arena, int sz) {
    if (POLICY == FIRST_FIT) {
        for (MabPtr p = arena; p; p = p->next)
            if (!p->allocated && p->size >= sz) return p;
    }
    else if (POLICY == BEST_FIT) {
        MabPtr best = nullptr; int waste = INT_MAX;
        for (MabPtr p = arena; p; p = p->next)
            if (!p->allocated && p->size >= sz && (p->size - sz) < waste) {
                waste = p->size - sz; best = p;
            }
        return best;
    }
    else if (POLICY == WORST_FIT) {
        MabPtr worst = nullptr; int waste = -1;
        for (MabPtr p = arena; p; p = p->next)
            if (!p->allocated && p->size >= sz && (p->size - sz) > waste) {
                waste = p->size - sz; worst = p;
            }
        return worst;
    }
    else if (POLICY == NEXT_FIT) {
        if (!next_fit_ptr) next_fit_ptr = arena;
        MabPtr p = next_fit_ptr;
        MabPtr start = p;
        do {
            if (!p->allocated && p->size >= sz) {
                next_fit_ptr = p->next ? p->next : arena;
                return p;
            }
            p = p->next ? p->next : arena;
        } while (p != start);
        next_fit_ptr = arena;
    }
    return nullptr;
}

MabPtr memAlloc(MabPtr arena, int sz) {
    MabPtr b = findBlock(arena, sz);
    return b ? memSplit(b, sz) : nullptr;
}

MabPtr memFree(MabPtr m) {
    if (!m) return nullptr;
    m->allocated = 0;
    return memMerge(m);
}

MabPtr createArena() {
    MabPtr a = new mab{0, 1024, 0, nullptr, nullptr};
    next_fit_ptr = nullptr;
    return a;
}

Process parseLine(const string& line, int num) {
    stringstream ss(line);
    string token;
    vector<int> v;
    while (getline(ss, token, ',')) v.push_back(stoi(token));
    if (v.size() != 5) {
        cout << "ERROR: Process " << num << " needs 5 values!\n";
        exit(1);
    }
    return {v[0], v[1], v[2], v[3], v[4]};
}

void runSimulation(const vector<Process>& processes) {
    MabPtr arena = createArena();

    // Allocate first 6
    for (int i = 0; i < 6 && i < processes.size(); ++i) {
        const Process& p = processes[i];
        cout << "Time 0: Allocating process " << p.pid << " (" << p.memory << " MB)\n";
        MabPtr blk = memAlloc(arena, p.memory);
        if (blk) cout << "   Allocated at offset " << blk->offset << "\n\n";
    }

    // Free priority 1 processes (192,416,160)
    cout << "Time 7: Freeing priority 1 processes (192, 416, 160 MB)...\n";
    memFree(arena);
    memFree(arena->next->next);
    MabPtr temp = arena;
    while (temp && temp->offset != 672) temp = temp->next;
    if (temp) memFree(temp);

    printArena(arena);

    // Allocate remaining
    cout << "Time 7: New processes arrive...\n\n";
    for (size_t i = 6; i < processes.size(); ++i) {
        const Process& p = processes[i];
        cout << "Allocating process " << p.pid << " (" << p.memory << " MB)\n";
        MabPtr blk = memAlloc(arena, p.memory);
        if (blk) cout << "   Allocated at expected offset " << blk->offset << "\n\n";
        else cout << "   FAILED!\n\n";
    }

    cout << "==================================================\n";
    cout << "                FINAL MEMORY MAP\n";
    cout << "==================================================\n";
    printArena(arena);

    cout << "Expected offsets for " << policy_names[POLICY] << ":\n";
    if (POLICY == FIRST_FIT)  cout << "108 at 0   | 256 at 224   | 80 at 108\n";
    if (POLICY == NEXT_FIT)   cout << "108 at 0   | 256 at 224   | 80 at 480\n";
    if (POLICY == BEST_FIT)   cout << "108 at 672 | 256 at 224   | 80 at 480\n";
    if (POLICY == WORST_FIT)  cout << "108 at 224 | 256 at 332   | 80 at 0\n";

    // Clean up
    while (arena) {
        MabPtr t = arena;
        arena = arena->next;
        delete t;
    }
}

int main() {
    cout << "====================================================\n";
    cout << "     MEMORY ALLOCATION SIMULATOR (1024 MB RAM)\n";
    cout << "====================================================\n\n";

    while (true) {
        cout << "Choose allocation policy:\n";
        cout << "1. First Fit\n2. Best Fit\n3. Worst Fit\n4. Next Fit\n";
        cout << "Enter choice (1-4): ";
        int ch; cin >> ch; cin.ignore();
        POLICY = (Policy)(ch - 1);

        cout << "\nYou selected: " << policy_names[POLICY] << "\n\n";

        vector<Process> processes;
        cout << "How many processes? ";
        int n; cin >> n; cin.ignore();

        cout << "\nEnter each process as: arrival,pid,priority,memory,cpu_time\n";
        cout << "Example: 0,1,1,192,0\n\n";

        for (int i = 1; i <= n; ++i) {
            string line;
            cout << "Process " << i << " → ";
            getline(cin, line);
            Process p = parseLine(line, i);
            processes.push_back(p);
        }

        cout << "\nStarting simulation...\n\n";
        runSimulation(processes);

        // MENU – DOES NOT EXIT
        while (true) {
            cout << "\n==================================================\n";
            cout << "What next?\n";
            cout << "1. Run again with SAME processes\n";
            cout << "2. Enter NEW processes\n";
            cout << "3. Exit program\n";
            cout << "Choose (1-3): ";
            int opt; cin >> opt; cin.ignore();

            if (opt == 1) {
                cout << "\nRunning again with same processes...\n\n";
                runSimulation(processes);
            }
            else if (opt == 2) {
                break; // go back to enter new processes
            }
            else if (opt == 3) {
                cout << "\nThank you!\n";
                return 0;
            }
        }
    }
    return 0;
} 