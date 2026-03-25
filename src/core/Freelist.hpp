#include <cassert>

template <uint32_t Capacity>
class FreeList {
public:
    FreeList() {
        stack.reserve(Capacity);
        for (uint32_t i = 0; i < Capacity; i++) {
            stack.push_back(Capacity - 1 - i);
        }
        count = Capacity-1;
    }

    uint32_t allocate() {
        assert(count != UINT32_MAX);
        return stack[count--];
    }

    void deallocate(uint32_t idx) {
        assert(count < Capacity - 1);
        stack[++count] = idx;
    }

private:
    std::vector<uint32_t> stack;
    uint32_t count;
};