#include <iostream>
#include <vector>

template <bool B, typename T, typename F>
struct conditional {
  using type = F;
};

template <typename T, typename F>
struct conditional<true, T, F> {
  using type = T;
};

template <bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

template <typename T>
class Deque {
 private:
  static const size_t RowSize = 32;
  size_t size_ = 0;
  size_t num_columns_ = 1;
  size_t head_column_index_ = 0;
  size_t head_row_index_ = 0;
  size_t tail_column_index_ = 0;
  size_t tail_row_index_ = 0;
  T** arr_;

  template <bool IsConst>
  class common_iterator {
   private:
    T** ptr = nullptr;
    int pos = 0;

   public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using iterator_category = std::random_access_iterator_tag;
    using pointer = conditional_t<IsConst, const T*, T*>;
    using reference = conditional_t<IsConst, const T&, T&>;
    common_iterator(T*& ptr, int pos) : ptr(&ptr), pos(pos) {}

    common_iterator(const common_iterator& copy)
        : ptr(copy.ptr), pos(copy.pos) {}

    common_iterator& operator=(const common_iterator& other) {
      common_iterator copy(other);
      swap(copy);
      return *this;
    }

    void swap(common_iterator& copy) {
      std::swap(ptr, copy.ptr);
      std::swap(pos, copy.pos);
    }

    common_iterator& operator++() {
      if (pos + 1 == RowSize) {
        ++ptr;
      }
      pos = (pos + 1) % RowSize;
      return *this;
    }

    common_iterator& operator--() {
      if (pos == 0) {
        --ptr;
      }
      pos = (pos + RowSize - 1) % RowSize;
      return *this;
    }

    common_iterator operator++(int) {
      common_iterator copy = *this;
      ++(*this);
      return copy;
    }

    common_iterator operator--(int) {
      common_iterator copy = *this;
      --(*this);
      return copy;
    }

    common_iterator& operator+=(int num) {
      if (num >= 0) {
        if (num + pos >= static_cast<int>(RowSize)) {
          ptr += (num + pos) / RowSize;
        }
        pos = (num + pos) % RowSize;
      } else {
        num = -num;
        if (num > pos) {
          ptr -= (num - pos + RowSize - 1) / RowSize;
        }
        pos = (pos - num) % RowSize;
      }
      return *this;
    }

    common_iterator operator+(int num) const {
      common_iterator copy = *this;
      copy += num;
      return copy;
    }

    common_iterator operator-(int num) const {
      common_iterator copy = *this;
      copy -= num;
      return copy;
    }

    common_iterator& operator-=(int num) {
      *this += -num;
      return *this;
    }

    bool operator==(const common_iterator<IsConst>& other) const {
      return ptr == other.ptr && pos == other.pos;
    }

    bool operator<(const common_iterator<IsConst>& other) const {
      if (ptr == other.ptr) {
        return pos < other.pos;
      }
      return ptr < other.ptr;
    }

    bool operator!=(const common_iterator<IsConst>& other) const {
      return !(*this == other);
    }

    bool operator>(const common_iterator<IsConst>& other) const {
      return other < *this;
    }

    bool operator<=(const common_iterator<IsConst>& other) const {
      return *this == other || *this < other;
    }

    bool operator>=(const common_iterator<IsConst>& other) const {
      return *this == other || *this > other;
    }

    reference operator*() const { return *((*ptr) + pos); }

    pointer operator->() const { return &((*ptr)[pos]); }

    difference_type operator-(const common_iterator<IsConst>& other) const {
      return (ptr - other.ptr) * RowSize + (pos - other.pos);
    }

    operator common_iterator<true>() { return common_iterator<true>(ptr, pos); }
  };

  void reserve(size_t num_columns) {
      arr_ = new T*[num_columns];
      size_t i;
      try {
          for (i = 0; i < num_columns; ++i) {
              arr_[i] = reinterpret_cast<T*>(new uint8_t[sizeof(T) * RowSize]);
          }
      } catch(...) {
          for (size_t j = 0; j < i; ++j) {
              delete[] reinterpret_cast<uint8_t*>(arr_[j]);
          }
          delete[] arr_;
          throw;
      }
  }

 public:
  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using reverse_const_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() {
    return iterator(arr_[head_row_index_], head_column_index_);
  }

  iterator end() { return iterator(arr_[tail_row_index_], tail_column_index_); }

  const_iterator cbegin() const {
    return const_iterator(arr_[head_row_index_], head_column_index_);
  }
  const_iterator begin() const {
    return const_iterator(arr_[head_row_index_], head_column_index_);
  }

  const_iterator cend() const {
    return const_iterator(arr_[tail_row_index_], tail_column_index_);
  }

  const_iterator end() const {
    return const_iterator(arr_[tail_row_index_], tail_column_index_);
  }

  reverse_iterator rbegin() {
      return std::make_reverse_iterator(end());
  }

  reverse_iterator rend() {
      return std::make_reverse_iterator(begin());
  }

  reverse_const_iterator crbgein() const {
    return std::make_reverse_iterator(cend());
  }

  reverse_const_iterator rbgein() const {
    return std::make_reverse_iterator(cend());
  }

  reverse_const_iterator crend() const {
    return std::make_reverse_iterator(cbegin());
  }

  reverse_const_iterator rend() const {
    return std::make_reverse_iterator(cbegin());
  }

  Deque() {
    arr_ = new T*[1];
    try {
        arr_[0] = reinterpret_cast<T*>(new uint8_t[sizeof(T) * RowSize]);
    } catch(...) {
        delete[] arr_;
    }
  }

  Deque(const Deque& other)
      : size_(other.size_),
        num_columns_(other.num_columns_),
        head_column_index_(other.head_column_index_),
        head_row_index_(other.head_row_index_),
        tail_column_index_(other.tail_column_index_),
        tail_row_index_(other.tail_row_index_) {
    reserve(num_columns_);
    size_t count = 0;
    try {
      for (size_t i = head_row_index_; i <= tail_row_index_; ++i) {
        size_t first = (i == head_row_index_) ? head_column_index_ : 0;
        size_t last = (i == tail_row_index_) ? tail_column_index_ : RowSize;
        for (size_t j = first; j < last; ++j) {
          new (arr_[i] + j) T(other.arr_[i][j]);
          ++count;
        }
      }
    } catch (...) {
      for (size_t i = 0; i < count; ++i) {
        (*this)[i].~T();
      }
      for (size_t i = 0; i < other.num_columns_; ++i) {
        delete[] reinterpret_cast<uint8_t*>(arr_[i]);
      }
      delete[] arr_;
      throw;
    }
  }

  void swap(Deque& other) {
    std::swap(head_column_index_, other.head_column_index_);
    std::swap(head_row_index_, other.head_row_index_);
    std::swap(tail_column_index_, other.tail_column_index_);
    std::swap(tail_row_index_, other.tail_row_index_);
    std::swap(num_columns_, other.num_columns_);
    std::swap(size_, other.size_);
    std::swap(arr_, other.arr_);
  }

  Deque<T>& operator=(const Deque& other) {
    Deque<T> copy(other);
    swap(copy);
    return *this;
  }

  Deque(size_t num, const T& value = T())
      : size_(num),
        num_columns_((num / RowSize + 1) * 3),
        head_column_index_(0),
        head_row_index_(num_columns_ / 2),
        tail_column_index_(num % RowSize),
        tail_row_index_(head_row_index_ + num / RowSize) {
      reserve(num_columns_);
    size_t count = 0;
    try {
      for (size_t i = head_row_index_; i <= tail_row_index_; ++i) {
        size_t first = (i == head_row_index_) ? head_column_index_ : 0;
        size_t last = (i == tail_row_index_) ? tail_column_index_ : RowSize;
        for (size_t j = first; j < last; ++j) {
          new (arr_[i] + j) T(value);
          ++count;
        }
      }
    } catch (...) {
      for (size_t i = 0; i < count; ++i) {
        (*this)[i].~T();
      }
      for (size_t i = 0; i < num_columns_; ++i) {
        delete[] reinterpret_cast<uint8_t*>(arr_[i]);
      }
      delete[] arr_;
      throw;
    }
  }

  size_t size() const { return size_; }

  T& operator[](size_t index) {
    return arr_[head_row_index_ + (head_column_index_ + index) / RowSize]
               [(head_column_index_ + index) % RowSize];
  }

  const T& operator[](size_t index) const {
    return arr_[head_row_index_ + (head_column_index_ + index) / RowSize]
               [(head_column_index_ + index) % RowSize];
  }

  T& at(size_t index) {
    if (index >= size()) {
      throw std::out_of_range("");
    }
    return (*this)[index];
  }

  const T& at(size_t index) const {
    if (index >= size()) {
      throw std::out_of_range("");
    }
    return (*this)[index];
  }

  void reallocate() {
    T** new_deque = new T*[num_columns_ * 3];
    size_t i;
    try {
        for (i = 0; i < num_columns_ * 3; ++i) {
            if (i < num_columns_ || i >= 2 * num_columns_) {
                new_deque[i] = reinterpret_cast<T*>(new uint8_t[sizeof(T) * RowSize]);
            } else {
                new_deque[i] = arr_[i - num_columns_];
            }
        }
    } catch(...) {
        for (size_t j = 0; j < i; ++j) {
            if (j < num_columns_ || j >= 2 * num_columns_) {
                delete[] reinterpret_cast<uint8_t*>(new_deque[j]);
            }
        }
        delete[] new_deque;
        throw;
    }

    delete[] arr_;

    arr_ = new_deque;
    head_row_index_ += num_columns_;
    tail_row_index_ += num_columns_;
    num_columns_ *= 3;
  }

  void push_back(const T& value) {
    new (arr_[tail_row_index_] + tail_column_index_) T(value);
    if (tail_column_index_ + 1 == RowSize) {
      if (tail_row_index_ + 1 == num_columns_) {
        try {
            reallocate();
        } catch(...) {
            arr_[tail_row_index_][tail_column_index_].~T();
            throw;
        }
      }
      ++tail_row_index_;
      tail_column_index_ = 0;
    } else {
      ++tail_column_index_;
    }
    ++size_;
  }

  void push_front(const T& value) {
    if (head_column_index_!= 0 || head_row_index_ != 0) {
        new (arr_[head_row_index_ - (head_column_index_ == 0)] + (head_column_index_ + RowSize - 1) % RowSize) T(value);
        if (head_column_index_ == 0) {
            --head_row_index_;
            head_column_index_ = RowSize - 1;
        } else {
            --head_column_index_;
        }
    } else {
        Deque copy = *this;
        reallocate();
        try {
            new (arr_[head_row_index_ - 1] + RowSize - 1) T(value);
            --head_row_index_;
            head_column_index_ = RowSize - 1;
        } catch(...) {
            swap(copy);
            throw;
        }
    }
    ++size_;
  }

  void pop_back() {
    if (tail_column_index_ == 0) {
      --tail_row_index_;
      tail_column_index_ = RowSize - 1;
    } else {
        --tail_column_index_;
    }
    arr_[tail_row_index_][tail_column_index_].~T();
    --size_;
  }

  void pop_front() {
    arr_[head_row_index_][head_column_index_].~T();
    if (head_column_index_ + 1 == RowSize) {
      ++head_row_index_;
      head_column_index_ = 0;
    } else {
        ++head_column_index_;
    }
    --size_;
  }

  void insert(iterator it, const T& value) {
    Deque<T> copy = *this;
    try {
      T tmp = value;
      for (; it != end(); ++it) {
        std::swap(*it, tmp);
      }
      push_back(tmp);
    } catch (...) {
      swap(copy);
      throw;
    }
  }

  void erase(iterator it) {
    Deque<T> copy = *this;
    try {
      for (; it + 1 != end(); ++it) {
        std::swap(*it, *(it + 1));
      }
      pop_back();
    } catch (...) {
      swap(copy);
      throw;
    }
  }

  ~Deque() {
    for (size_t i = 0; i < size(); ++i) {
      (*this)[i].~T();
    }
    for (size_t i = 0; i < num_columns_; ++i) {
      delete[] reinterpret_cast<uint8_t*>(arr_[i]);
    }
    delete[] arr_;
  }
};
