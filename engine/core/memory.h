/**
 * ACE Engine — Memory Management
 * Arena (bump) allocator for per-frame allocations.
 * Pool allocator for fixed-size object recycling.
 * Stack allocator for LIFO scratch memory.
 *
 * All allocators are designed for zero-overhead, cache-friendly patterns.
 */

#pragma once

#include "types.h"
#include <new>
#include <utility>
#include <memory>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace ace {

// ============================================================================
// ARENA ALLOCATOR — Bump allocator with reset, ideal for per-frame temps
// ============================================================================
class ArenaAllocator {
public:
    explicit ArenaAllocator(size_t capacityBytes = 1024 * 1024)
        : _capacity(capacityBytes), _offset(0)
    {
        _buffer = static_cast<u8*>(std::malloc(capacityBytes));
        assert(_buffer && "ArenaAllocator: allocation failed");
    }

    ~ArenaAllocator() { std::free(_buffer); }

    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    ArenaAllocator(ArenaAllocator&& other) noexcept
        : _buffer(other._buffer), _capacity(other._capacity), _offset(other._offset)
    {
        other._buffer = nullptr;
        other._capacity = 0;
        other._offset = 0;
    }

    void* Alloc(size_t size, size_t alignment = alignof(std::max_align_t)) {
        size_t aligned = AlignUp(_offset, alignment);
        if (aligned + size > _capacity) return nullptr; // Out of memory
        void* ptr = _buffer + aligned;
        _offset = aligned + size;
        return ptr;
    }

    template<typename T, typename... Args>
    T* New(Args&&... args) {
        void* mem = Alloc(sizeof(T), alignof(T));
        return mem ? new(mem) T(std::forward<Args>(args)...) : nullptr;
    }

    template<typename T>
    T* AllocArray(size_t count) {
        void* mem = Alloc(sizeof(T) * count, alignof(T));
        if (!mem) return nullptr;
        for (size_t i = 0; i < count; ++i) new(static_cast<T*>(mem) + i) T{};
        return static_cast<T*>(mem);
    }

    void Reset() { _offset = 0; }

    size_t Used()     const { return _offset; }
    size_t Capacity() const { return _capacity; }
    size_t Free()     const { return _capacity - _offset; }

private:
    static size_t AlignUp(size_t offset, size_t alignment) {
        return (offset + alignment - 1) & ~(alignment - 1);
    }

    u8*    _buffer;
    size_t _capacity;
    size_t _offset;
};

// ============================================================================
// POOL ALLOCATOR — Fixed-size object pool with free list
// ============================================================================
template<typename T>
class PoolAllocator {
    union Slot {
        T       value;
        Slot*   next;
        Slot() {}
        ~Slot() {}
    };

public:
    explicit PoolAllocator(size_t blockSize = 256)
        : _blockSize(blockSize), _freeList(nullptr) {}

    ~PoolAllocator() {
        for (auto* block : _blocks) std::free(block);
    }

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    template<typename... Args>
    T* Acquire(Args&&... args) {
        if (!_freeList) AllocBlock();
        Slot* slot = _freeList;
        _freeList = slot->next;
        return new(&slot->value) T(std::forward<Args>(args)...);
    }

    void Release(T* obj) {
        obj->~T();
        Slot* slot = reinterpret_cast<Slot*>(obj);
        slot->next = _freeList;
        _freeList = slot;
    }

private:
    void AllocBlock() {
        Slot* block = static_cast<Slot*>(std::malloc(sizeof(Slot) * _blockSize));
        assert(block && "PoolAllocator: allocation failed");
        _blocks.push_back(block);

        for (size_t i = 0; i < _blockSize - 1; ++i)
            block[i].next = &block[i + 1];
        block[_blockSize - 1].next = _freeList;
        _freeList = block;
    }

    size_t             _blockSize;
    Slot*              _freeList;
    std::vector<Slot*> _blocks;
};

// ============================================================================
// STACK ALLOCATOR — LIFO with save/restore markers
// ============================================================================
class StackAllocator {
public:
    using Marker = size_t;

    explicit StackAllocator(size_t capacityBytes = 512 * 1024)
        : _capacity(capacityBytes), _offset(0)
    {
        _buffer = static_cast<u8*>(std::malloc(capacityBytes));
        assert(_buffer && "StackAllocator: allocation failed");
    }

    ~StackAllocator() { std::free(_buffer); }

    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;

    void* Alloc(size_t size, size_t alignment = alignof(std::max_align_t)) {
        size_t aligned = (_offset + alignment - 1) & ~(alignment - 1);
        if (aligned + size > _capacity) return nullptr;
        void* ptr = _buffer + aligned;
        _offset = aligned + size;
        return ptr;
    }

    Marker Save()            const { return _offset; }
    void   Restore(Marker m)       { _offset = m; }
    void   Reset()                 { _offset = 0; }

private:
    u8*    _buffer;
    size_t _capacity;
    size_t _offset;
};

// ============================================================================
// SCOPED ARENA — RAII marker for ArenaAllocator
// ============================================================================
class ScopedArena {
public:
    explicit ScopedArena(ArenaAllocator& arena)
        : _arena(arena), _marker(arena.Used()) {}

    ~ScopedArena() { _arena.Reset(); /* simple reset, no partial restore needed for arena */ }

    void* Alloc(size_t size) { return _arena.Alloc(size); }

    template<typename T, typename... Args>
    T* New(Args&&... args) { return _arena.New<T>(std::forward<Args>(args)...); }

private:
    ArenaAllocator& _arena;
    size_t          _marker;
};

} // namespace ace
