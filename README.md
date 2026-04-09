# CS3339 Cache Simulator (HW3)

## Author

Ricky Mosqueda-Torres

---

## Description

This project implements a cache simulator for CS3339. The simulator models the behavior of a set-associative cache given a sequence of memory references.
For each memory access, the simulator determines whether the access is a **HIT** or **MISS**.
This implementation also includes all extra credit features:

* Multi-word blocks
* Multi-level cache (L1 + L2)
* Miss classification:

  * Compulsory
  * Conflict
  * Capacity

---

## Features

### Base Requirements

* Configurable number of entries
* Configurable associativity
* Reads memory references from a file
* Outputs HIT or MISS per reference
* Uses LRU replacement policy

### Extra Credit Implemented

* Multi-word blocks (block size > 1)
* Multi-level cache (L1 + optional L2)
* Miss classification:

  * Compulsory (first access)
  * Conflict (would hit in fully associative cache)
  * Capacity (miss even in fully associative)

---

## Build Instructions

Compile the program using:

```bash
g++ -std=c++17 -o cache_sim HW3.cpp
```

---

## Run Instructions

### 1. Base Assignment (L1 only)

```bash
.\cache_sim <num_entries> <associativity> <memory_reference_file>
```

Example:

```bash
.\cache_sim 4 2 memory_reference_file.txt
```

---

### 2. Multi-word Blocks

```bash
.\cache_sim <num_entries> <associativity> <memory_reference_file> <block_size>
```

Example:

```bash
.\cache_sim 4 2 memory_reference_file.txt 2
```

---

### 3. Multi-level Cache (L1 + L2)

```bash
.\cache_sim <L1_entries> <L1_assoc> <memory_file> <block_size> <L2_entries> <L2_assoc>
```

Example:

```bash
.\cache_sim 4 2 memory_reference_file.txt 1 8 4
```

---

## Output

Results are written to:

```
cache_sim_output
```

Each line format (base requirement):

```
ADDR : HIT/MISS
```

Extended output (extra credit):

```
ADDR : MISS [COMPULSORY]
ADDR : MISS (L1), HIT (L2) [CONFLICT]
```

---

## Test Cases (From My Terminal)

### Test 1: Basic Hits

Input (`memory_reference_file.txt`):

```
0 1 0 1 0 1
```

Run:

```bash
.\cache_sim 4 2 memory_reference_file.txt
type cache_sim_output
```

Output:

```
0 : MISS [COMPULSORY]
1 : MISS [COMPULSORY]
0 : HIT
1 : HIT
0 : HIT
1 : HIT
```

---

### Test 2: Conflict Miss

Input:

```
0 2 4 0
```

Run:

```bash
.\cache_sim 4 2 memory_reference_file.txt
type cache_sim_output
```

Output:

```
0 : MISS [COMPULSORY]
2 : MISS [COMPULSORY]
4 : MISS [COMPULSORY]
0 : MISS [CONFLICT]
```

---

### Test 3: Multi-word Blocks

Input:

```
0 1 2 3 0 1 2 3
```

Run:

```bash
.\cache_sim 4 2 memory_reference_file.txt 2
type cache_sim_output
```

Output:

```
0 : MISS [COMPULSORY]
1 : HIT
2 : MISS [COMPULSORY]
3 : HIT
0 : HIT
1 : HIT
2 : HIT
3 : HIT
```

---

### Test 4: Multi-level Cache (L1 + L2)

Input:

```
0 2 4 0
```

Run:

```bash
.\cache_sim 4 2 memory_reference_file.txt 1 8 4
type cache_sim_output
```

Output:

```
0 : MISS (L1), MISS (L2) [COMPULSORY]
2 : MISS (L1), MISS (L2) [COMPULSORY]
4 : MISS (L1), MISS (L2) [COMPULSORY]
0 : MISS (L1), HIT (L2) [CONFLICT]
```

---

## Known Limitations

* Miss classification is applied to L1
* LRU replacement policy only
