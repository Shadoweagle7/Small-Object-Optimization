// Small Object Optimization.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <cstddef>
#include <concepts>
#include <stdexcept>

class canary_value_corruption_detected : public std::runtime_error {
public:
	canary_value_corruption_detected() : std::runtime_error("Canary value corruption detected") {}
};

template<class T, size_t CANARY_VALUE_SIZE, size_t SIZE_THRESHOLD>
union raw_storage {
	std::byte stack[CANARY_VALUE_SIZE + sizeof(T) + CANARY_VALUE_SIZE];
	std::byte *heap;
};

template<class T, size_t CANARY_VALUE_SIZE, size_t SIZE_THRESHOLD>
class storage {
private:
	raw_storage<T, CANARY_VALUE_SIZE, SIZE_THRESHOLD> rs;
public:
	static constexpr bool should_stack_allocate = sizeof(T) <= SIZE_THRESHOLD;
	static constexpr bool is_stack_allocated = should_stack_allocate;
	static constexpr bool should_heap_allocate = sizeof(T) > SIZE_THRESHOLD;
	static constexpr bool is_heap_allocated = should_heap_allocate;

	template<class... argv>
	storage(argv&&... args) 
		requires 
		std::constructible_from<T, argv...> || // TODO: ?
		std::constructible_from<T, argv&&...> {
		if constexpr (should_heap_allocate) {
			this->rs.heap = new std::byte[CANARY_VALUE_SIZE + sizeof(T) + CANARY_VALUE_SIZE];
			void *nodiscard = ::operator new(sizeof(T), this->rs.heap + CANARY_VALUE_SIZE);

			std::cout << "Heap allocated new T at " << nodiscard << "\n";
		} else {
			T *ptr = reinterpret_cast<T *>(&this->rs.stack[0] + CANARY_VALUE_SIZE);
			void *nodiscard = ::operator new(sizeof(T), ptr);

			std::cout << 
				"Stack allocated new T at " << 
				nodiscard << 
			"\n";
		}
	}

	void check_canary_values(
		bool immediate_std_terminate_on_stack_canary_corruption = true
	) requires (CANARY_VALUE_SIZE > 0) {

	}

	storage &operator=(const T &val) requires std::copy_constructible<T> {
		if constexpr (is_heap_allocated) {
			T &ref_data = *reinterpret_cast<T *>(this->rs.heap + CANARY_VALUE_SIZE);

			ref_data = val;
		} else {
			T &ref_data = *reinterpret_cast<T *>(&this->rs.stack[0] + CANARY_VALUE_SIZE);

			ref_data = val;
		}

		return *this;
	}

	operator T &() {
		if constexpr (is_heap_allocated) {
			return *reinterpret_cast<T *>(this->rs.heap + CANARY_VALUE_SIZE);
		} else {
			return *reinterpret_cast<T *>(&this->rs.stack[0] + CANARY_VALUE_SIZE);
		}
	}

	~storage() {
		if constexpr (is_heap_allocated) {
			delete[] this->rs.heap;

			std::cout << 
				"Heap allocated memory for T at " << 
				(this->rs.heap + CANARY_VALUE_SIZE) << 
			" freed\n";
		}
	}
};

int main() {
	storage<int, 0, 4> stack_test;
	storage<int, 0, 1> heap_test;

	stack_test = 27;
	heap_test = 27;

	std::cout << "[TEST]: stack_test = " << stack_test << " | heap_test = " << heap_test << "\n";

	storage<int, 4, 4> stack_with_canary_4_test;
	storage<int, 4, 1> heap_with_canary_4_test;

	stack_with_canary_4_test = 36;
	heap_with_canary_4_test = 36;

	std::cout <<
		"[TEST]: "
		"stack_with_canary_4_test = " << 
		stack_with_canary_4_test << 
		" | heap_with_canary_4_test = " << 
		heap_with_canary_4_test << 
	"\n";

	std::cout << "[TEST]: Attempting to test smashing stack on storage with canary values...\n";

	std::cout << "[TEST]: Test complete.\n";



	std::cout << "[TEST]: Attempting to test smashing heap on storage with canary values...\n";

	std::cout << "[TEST]: Test complete.\n";

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
