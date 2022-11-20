#ifndef QUEUE_INCLUDED
#define QUEUE_INCLUDED

#include <stddef.h>
#include <stdint.h>

#include "error_code_groups.hpp"
#include "data/error_or.hpp"

namespace Kernel::Data {

    enum QueueErrorCode : int {
        QUEUE_IS_FULL = ErrorCodeGroup::get_id(ErrorCodeGroup::Group::DATA_QUEUE),
        QUEUE_IS_EMPTY,
    };

    template <typename T, size_t SIZE>
    class Queue {
    private:
        static constexpr size_t next_position(size_t t_pos) {
            return (t_pos + 1) % (SIZE + 1);
        }

    public:
        Queue()
            : m_startIndex(0)
            , m_endIndex(0)
        {
            ;
        }

        int push_back(T t_element) {
            if (is_full()) {
                return QUEUE_IS_FULL;
            }
            m_data[m_endIndex] = t_element;
            m_endIndex = next_position(m_endIndex);
            return 0;
        }
        
        Data::ErrorOr<T> pop_front() {
            if (is_empty()) {
                return QUEUE_IS_EMPTY;
            }

            const T result = m_data[m_startIndex];
            m_startIndex = next_position(m_startIndex);
            return result;
        }

        [[nodiscard]] bool is_empty() const {
            return (m_endIndex == m_startIndex);
        }

        [[nodiscard]] bool is_full() const {
            return (next_position(m_endIndex) == m_startIndex);
        }

        [[nodiscard]] size_t size() const {
            if (m_endIndex < m_startIndex) {
                return (m_endIndex + SIZE) - m_startIndex;
            }
            else {
                return m_endIndex - m_startIndex;
            }
        }

        static constexpr size_t capacity() {
            return SIZE;
        }

    private:
        T m_data[SIZE + 1]; // without the extra space when start = end we wouldnt be able to tell if the buffer is full or empty
        size_t m_startIndex;
        size_t m_endIndex;
    };

}

#endif
