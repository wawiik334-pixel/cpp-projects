#ifndef HashTable_H
#define HashTable_H

#include <cstring>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <new>

inline size_t data_hash(const std::string& k) {
	size_t hash = 5381;
	for (char c : k) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

template <class Key, class Value>
class HashTable {
	struct TableEntry {
		Key k;
		Value v;
		bool used;
	};

	// Iterator implementation
	class HTIterator {
		size_t entries_left;
		struct TableEntry* table;
	public:
		HTIterator(TableEntry* t, size_t el) {
			table = t;
			entries_left = el;
			while (entries_left > 0 && !table[entries_left - 1].used)
				entries_left--;
		}
		HTIterator operator++() {
			if (entries_left == 0)
				return *this;
			entries_left--;
			while (entries_left > 0 && !table[entries_left - 1].used) {
				entries_left--;
			}
			return *this;
		}
		bool operator!=(const HTIterator& other) {
			return other.entries_left != entries_left;
		}
		Key& operator*() {
			return table[entries_left - 1].k;
		}
	};

	struct TableEntry* table;
	size_t capacity = 10;
	size_t items = 0;

public:
	// Constructors: Manually set 'used' flag to false.
	HashTable() {
		table = (struct TableEntry*)malloc(sizeof(struct TableEntry) * 10);
		for (size_t i = 0; i < 10; ++i) {
			table[i].used = false;
		}
	}
	HashTable(size_t initial_capacity) : capacity(initial_capacity) {
		table = (struct TableEntry*)malloc(sizeof(struct TableEntry) * capacity);
		for (size_t i = 0; i < capacity; ++i) {
			table[i].used = false;
		}
	}

	size_t get_items() const {
		return items;
	}

	HTIterator begin() {
		return HTIterator(table, capacity);
	}

	HTIterator end() {
		return HTIterator(table, 0);
	}

	bool contains(const Key& k) const {
		size_t hv = data_hash(k) % capacity;
		size_t attempts = 0;

		while (attempts < capacity) {
			if (!table[hv].used) {
				return false;
			}
			if (table[hv].k == k) {
				return true;
			}

			hv = (hv + 1) % capacity;
			attempts++;
		}
		return false;
	}

	Value& operator[] (const Key& k) {
		size_t hv = data_hash(k) % capacity;
		size_t attempts = 0;

		// Retrieval/Lookup
		while (attempts < capacity) {
			if (table[hv].used && table[hv].k == k) {
				return table[hv].v;
			}

			if (!table[hv].used) {
				break;
			}

			hv = (hv + 1) % capacity;
			attempts++;
		}

		// Resizing
		if (items > capacity / 2) {
			size_t new_capacity = capacity * 2;
			struct TableEntry* new_table = (struct TableEntry*)malloc(sizeof(struct TableEntry) * new_capacity);

			// Initialize 'used' flags in new table
			for (size_t i = 0; i < new_capacity; ++i) {
				new_table[i].used = false;
			}

			for (size_t i = 0; i < capacity; i++) { // each item in the old table
				if (!table[i].used)
					continue;

				// Re-hash/probe into the new table
				size_t new_hv = data_hash(table[i].k) % new_capacity;

				while (new_table[new_hv].used) {
					new_hv = (new_hv + 1) % new_capacity;
				}

				// Use Placement New to copy objects
				new (&new_table[new_hv].k) Key(table[i].k);
				new (&new_table[new_hv].v) Value(table[i].v);
				new_table[new_hv].used = true;

				// Destroy old objects
				table[i].k.~Key();
				table[i].v.~Value();
			}

			free(table);
			table = new_table;
			capacity = new_capacity;

			// Recalculate hash new table
			hv = data_hash(k) % capacity;
			attempts = 0;

			while (attempts < capacity) {
				if (!table[hv].used) {
					break;
				}
				hv = (hv + 1) % capacity;
				attempts++;
			}
		}

		new (&table[hv].k) Key(k);
		new (&table[hv].v) Value();
		table[hv].used = true;
		items++;
		return table[hv].v;
	}

	// Chain fixer/remove
	void chain_fixer(Key k, size_t pos) {
		size_t hv = data_hash(k) % capacity;
		long replacement = -1;
		size_t rsearch_pos = (pos + 1) % capacity;
		size_t attempts = 0;

		while (table[rsearch_pos].used && attempts < capacity) {
			if (hv == data_hash(table[rsearch_pos].k) % capacity) {
				replacement = rsearch_pos;
			}
			rsearch_pos = (rsearch_pos + 1) % capacity;
			attempts++;
		}

		if (replacement != -1) {
			// Destroy object at pos
			table[pos].k.~Key();
			table[pos].v.~Value();

			new (&table[pos].k) Key(table[replacement].k);
			new (&table[pos].v) Value(table[replacement].v);
			table[pos].used = table[replacement].used;

			// Destroy the replacement element
			table[replacement].k.~Key();
			table[replacement].v.~Value();
			table[replacement].used = false;
		}
	}

	void remove(Key k) {
		size_t hv = data_hash(k) % capacity;
		size_t attempts = 0;

		while (table[hv].used && attempts < capacity) {
			if (table[hv].k == k) {
				// Destroy the object
				table[hv].k.~Key();
				table[hv].v.~Value();
				table[hv].used = false;
				chain_fixer(k, hv);
				items--;
				return;
			}

			hv = (hv + 1) % capacity;
			attempts++;
		}
	}

	void debug_info() const {
		for (size_t i = 0; i < capacity; i++) {
			if (table[i].used)
				std::cout << i << ": " << table[i].k << std::endl;
			else
				std::cout << i << ": " << "unused\n";
		}
	}

	~HashTable() {
		// Call destructors for all used elements
		for (size_t i = 0; i < capacity; ++i) {
			if (table[i].used) {
				table[i].k.~Key();
				table[i].v.~Value();
			}
		}
		free(table);
	}
};
#endif