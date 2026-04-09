#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

// Cache class represents a set-associative cache
class Cache {
 public:
   // Result struct used to return hit/miss and eviction info
  struct Result {
    bool hit = false;
    bool evicted = false;
  };

  // Constructor initializes cache configuration
  Cache(int num_entries, int associativity, int block_size)
      : num_entries_(num_entries),
        associativity_(associativity),
        block_size_(block_size),
        num_sets_(num_entries / associativity),
        sets_(num_sets_, std::vector<Line>(associativity)) {}
  
  // Checks if a block is already in cache (used for lookups)
  bool Contains(int address) const {
    long long block_address = GetBlockAddress(address);
    int set_index = GetSetIndex(block_address);
    long long tag = GetTag(block_address);

    for (int i = 0; i < associativity_; ++i) {
      if (sets_[set_index][i].valid && sets_[set_index][i].tag == tag) {
        return true;
      }
    }
    return false;
  }

  // Main function to access cache (handles hit/miss and replacement)
  Result Access(int address) {
    long long block_address = GetBlockAddress(address);
    int set_index = GetSetIndex(block_address);
    long long tag = GetTag(block_address);

    // Check for HIT
    for (int i = 0; i < associativity_; ++i) {
      if (sets_[set_index][i].valid && sets_[set_index][i].tag == tag) {
        UpdateLru(set_index, i);
        return {true, false};
      }
    }

    // MISS: try to place in an empty (invalid) line first
    for (int i = 0; i < associativity_; ++i) {
      if (!sets_[set_index][i].valid) {
        sets_[set_index][i].valid = true;
        sets_[set_index][i].tag = tag;
        UpdateLru(set_index, i);
        return {false, false};
      }
    }

    // MISS: no empty line, must evict using LRU policy
    int lru_index = 0;
    for (int i = 1; i < associativity_; ++i) {
      if (sets_[set_index][i].lru_counter < sets_[set_index][lru_index].lru_counter) {
        lru_index = i;
      }
    }

    // Replace least recently used line
    sets_[set_index][lru_index].valid = true;
    sets_[set_index][lru_index].tag = tag;
    UpdateLru(set_index, lru_index);
    return {false, true};
  }

  // Getter functions
  int GetNumEntries() const { return num_entries_; }
  int GetAssociativity() const { return associativity_; }
  int GetBlockSize() const { return block_size_; }

 private:
   // Each cache line contains valid bit, tag, and LRU counter
  struct Line {
    bool valid = false;
    long long tag = -1;
    int lru_counter = 0;
  };

  // Convert address to block address (for multi-word blocks)
  long long GetBlockAddress(int address) const {
    return address / block_size_;
  }

  // Compute set index using modulo
  int GetSetIndex(long long block_address) const {
    return static_cast<int>(block_address % num_sets_);
  }

  // Compute tag from block address
  long long GetTag(long long block_address) const {
    return block_address / num_sets_;
  }

  // Update LRU counters after access
  void UpdateLru(int set_index, int accessed_index) {
    int old_value = sets_[set_index][accessed_index].lru_counter;

    // Decrease counters of more recently used lines
    for (int i = 0; i < associativity_; ++i) {
      if (sets_[set_index][i].valid &&
          sets_[set_index][i].lru_counter > old_value) {
        sets_[set_index][i].lru_counter--;
      }
    }

    // Set accessed line as most recently used
    sets_[set_index][accessed_index].lru_counter = associativity_ - 1;
  }

  int num_entries_;
  int associativity_;
  int block_size_;
  int num_sets_;
  std::vector<std::vector<Line>> sets_;
};

// Classifies miss type: compulsory, conflict, or capacity
std::string ClassifyMiss(
    int address,
    const Cache& actual_cache,
    Cache& fully_associative_same_capacity,
    std::unordered_set<long long>& seen_blocks) {
  long long block_address = address / actual_cache.GetBlockSize();

  // First time seeing this block → compulsory miss
  if (seen_blocks.find(block_address) == seen_blocks.end()) {
    seen_blocks.insert(block_address);
    fully_associative_same_capacity.Access(address);
    return "COMPULSORY";
  }

  // Check fully associative cache to distinguish conflict vs capacity
  bool fa_hit = fully_associative_same_capacity.Access(address).hit;
  if (fa_hit) {
    return "CONFLICT";
  }
  return "CAPACITY";
}

int main(int argc, char* argv[]) {
  // Program usage:
  // ./cache_sim <L1_entries> <L1_assoc> <memory_file>
  // ./cache_sim <L1_entries> <L1_assoc> <memory_file> <block_size>
  // ./cache_sim <L1_entries> <L1_assoc> <memory_file> <block_size> <L2_entries> <L2_assoc>

  if (argc != 4 && argc != 5 && argc != 7) {
    std::cerr << "Usage:\n";
    std::cerr << "  ./cache_sim <L1_entries> <L1_assoc> <memory_file>\n";
    std::cerr << "  ./cache_sim <L1_entries> <L1_assoc> <memory_file> <block_size>\n";
    std::cerr << "  ./cache_sim <L1_entries> <L1_assoc> <memory_file> <block_size> <L2_entries> <L2_assoc>\n";
    return 1;
  }

  // Parse L1 configuration
  int l1_entries = std::stoi(argv[1]);
  int l1_assoc = std::stoi(argv[2]);
  std::string memory_file_name = argv[3];

  // Default block size = 1 (single word blocks)
  int block_size = 1;
  if (argc == 5 || argc == 7) {
    block_size = std::stoi(argv[4]);
  }

  // Optional L2 cache configuration
  bool use_l2 = false;
  int l2_entries = 0;
  int l2_assoc = 0;
  if (argc == 7) {
    use_l2 = true;
    l2_entries = std::stoi(argv[5]);
    l2_assoc = std::stoi(argv[6]);
  }

  // Validate L1 config
  if (l1_entries <= 0 || l1_assoc <= 0 || block_size <= 0 ||
      l1_entries % l1_assoc != 0) {
    std::cerr << "Error: invalid L1 cache configuration.\n";
    return 1;
  }

  // Validate L2 config if used
  if (use_l2 && (l2_entries <= 0 || l2_assoc <= 0 || l2_entries % l2_assoc != 0)) {
    std::cerr << "Error: invalid L2 cache configuration.\n";
    return 1;
  }

  // Open input file containing memory references
  std::ifstream input_file(memory_file_name);
  if (!input_file.is_open()) {
    std::cerr << "Error: could not open memory reference file.\n";
    return 1;
  }

  // Create output file
  std::ofstream output_file("cache_sim_output");
  if (!output_file.is_open()) {
    std::cerr << "Error: could not create output file.\n";
    return 1;
  }

  // Initialize L1 cache and fully associative cache (for classification)
  Cache l1_cache(l1_entries, l1_assoc, block_size);
  Cache fully_associative_l1(l1_entries, l1_entries, block_size);

  // Initialize optional L2 cache
  Cache* l2_cache = nullptr;
  if (use_l2) {
    l2_cache = new Cache(l2_entries, l2_assoc, block_size);
  }

  // Track seen blocks for compulsory miss detection
  std::unordered_set<long long> seen_blocks;

  int address;

  // Process each memory reference
  while (input_file >> address) {
    Cache::Result l1_result = l1_cache.Access(address);

    // If L1 hit, output immediately
    if (l1_result.hit) {
      if (use_l2) {
        output_file << address << " : HIT (L1)\n";
      } else {
        output_file << address << " : HIT\n";
      }
      continue;
    }

    // Classify miss type
    std::string miss_type =
        ClassifyMiss(address, l1_cache, fully_associative_l1, seen_blocks);

    // If no L2, just output miss
    if (!use_l2) {
      output_file << address << " : MISS [" << miss_type << "]\n";
      continue;
    }

    // Check L2 cache
    Cache::Result l2_result = l2_cache->Access(address);

    if (l2_result.hit) {
      output_file << address << " : MISS (L1), HIT (L2) [" << miss_type << "]\n";
      // Bring block into L1 after L2 hit.
      l1_cache.Access(address);
    } else {
      output_file << address << " : MISS (L1), MISS (L2) [" << miss_type << "]\n";
      // On memory fetch, insert into both L2 and L1.
      l1_cache.Access(address);
    }
  }

  // Close files
  input_file.close();
  output_file.close();

  // Free L2 memory
  delete l2_cache;
  return 0;
}