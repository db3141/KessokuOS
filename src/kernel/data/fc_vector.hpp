#ifndef FIXED_CAPACITY_VECTOR_INCLUDED
#define FIXED_CAPACITY_VECTOR_INCLUDED

#include "common.hpp"
#include "data/error_or.hpp"

namespace Kernel::Data {

    template <typename T, size_t CAPACITY>
    class FCVector {
    public:
        FCVector() : m_size(0) {
            ;
        }

        Data::ErrorOr<void> insert(size_t t_index, const T& t_value) {
            ASSERT(t_index <= m_size, Error::INDEX_OUT_OF_RANGE);
            ASSERT(m_size < CAPACITY, Error::CONTAINER_IS_FULL);

            if (t_index < m_size) { 
                for (size_t i = m_size - 1; i > t_index; i--) {
                    m_array[i + 1] = m_array[i];
                }

                m_array[t_index + 1] = m_array[t_index];
            }

            m_array[t_index] = t_value;
            m_size++;

            return Data::ErrorOr<void>();
        }

        Data::ErrorOr<void> remove(size_t t_index) {
            ASSERT(t_index < m_size, Error::INDEX_OUT_OF_RANGE);

            for (size_t i = t_index; i < m_size; i++) {
                m_array[i] = m_array[i + 1];
            }

            m_size--;

            return Data::ErrorOr<void>();
        }

        Data::ErrorOr<void> push_back(const T& t_value) {
            return insert(m_size, t_value);
        }

        Data::ErrorOr<void> pop_back() {
            return remove(size() - 1);
        }

        T& operator[](size_t t_index) {
            return m_array[t_index]; // TODO: bounds check
        }

        const T& operator[](size_t t_index) const {
            return m_array[t_index];
        }

        [[nodiscard]] size_t size() const {
            return m_size;
        }

        [[nodiscard]] bool empty() const {
            return size() == 0;
        }

        [[nodiscard]] size_t capacity() const {
            return CAPACITY;
        }

    private:
        T m_array[CAPACITY];
        size_t m_size;
    };

}

#endif
