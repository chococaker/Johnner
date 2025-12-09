#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace choco {
    class Move {
    public:
        Move() { }

        Move(uint8_t pieceType, uint8_t from, uint8_t to)
                : pieceType(pieceType), from(from), to(to), promotionType(6) { }

        Move(uint8_t pieceType, uint8_t from, uint8_t to, uint8_t promotionType)
                : pieceType(pieceType), from(from), to(to), promotionType(promotionType) { }

        uint8_t pieceType;
        uint8_t from;
        uint8_t to;
        uint8_t promotionType;

        bool operator==(const Move& other) const;
    };

    class MoveList {
    public:
        MoveList();

        Move& operator[](size_t n);
        const Move& operator[](size_t n) const;
        
        uint8_t size() const;

        Move pop();
        void push_back(Move&& move);
        void push_back(const Move& move);
        void swap(uint8_t n1, uint8_t n2);

        class Iterator {
        public:
            Iterator(Move* ptr) : ptr(ptr) {}

            Move& operator*() { return *ptr; }
            const Move& operator*() const { return *ptr; }

            Move* operator->() { return ptr; }
            const Move* operator->() const { return ptr; }

            Iterator& operator++() {
                ++ptr;
                return *this;
            }

            Iterator operator++(int) {
                Iterator temp = *this;
                ++ptr;
                return temp;
            }

            Iterator& operator--() {
                --ptr;
                return *this;
            }

            Iterator operator--(int) {
                Iterator temp = *this;
                --ptr;
                return temp;
            }

            Iterator operator+(size_t n) const {
                return Iterator(ptr + n);
            }

            Iterator operator-(size_t n) const {
                return Iterator(ptr - n);
            }

            size_t operator-(const Iterator& other) const {
                return ptr - other.ptr;
            }

            bool operator==(const Iterator& other) const {
                return ptr == other.ptr;
            }

            bool operator!=(const Iterator& other) const {
                return ptr != other.ptr;
            }

        private:
            Move* ptr;
        };

        Iterator begin();
        Iterator end();
    private:
        static constexpr size_t MAX_MOVE_COUNT = 218;
        Move moves[MAX_MOVE_COUNT];
        uint8_t i;
    };
} // namespace choco
