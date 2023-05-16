#include <iostream>

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

template<size_t N>
class alignas(max_align_t) StackStorage {
private:
    uint8_t memory[N];
    size_t filled = 0;
public:
    StackStorage() = default;
    uint8_t* get_memory(size_t num, size_t align) {
        size_t pos = filled + (align - (filled % align)) % align;
        filled = pos + num;
        return memory + pos;
    }
};

template<typename T, size_t N>
class StackAllocator {
private:
    StackStorage<N>* ptr;
public:
    using value_type = T;

    StackAllocator() = delete;
    StackAllocator(const StackAllocator& other) = default;
    StackAllocator& operator= (const StackAllocator& other) = default;
    ~StackAllocator() = default;

    template<typename F, size_t M>
    friend class StackAllocator;

    explicit StackAllocator(StackStorage<N>& ptr): ptr(&ptr){}

    template<typename F>
    StackAllocator(const StackAllocator<F, N>& other): ptr(other.ptr) {}

    template<typename F>
    struct rebind {
        using other = StackAllocator<F, N>;
    };

    value_type* allocate(size_t num) {
        return reinterpret_cast<value_type*>(ptr->get_memory(num * sizeof(value_type), alignof(value_type)));
    }

    void deallocate(T*, size_t) {}

};

template<typename T, typename Alloc = std::allocator<T>>
class List {
private:
    struct BaseNode {
        BaseNode* prev;
        BaseNode* next;
        BaseNode() = default;
        BaseNode(BaseNode* prev, BaseNode* next): prev(prev), next(next) {}
    };

    struct Node: public BaseNode {
        T value_;
        explicit Node(const T& value): value_(value) {}

        template<typename ...Args>
        explicit Node(Args&&... args): value_(std::forward<Args>(args)...) {}
    };

    template<bool IsConst>
    class common_iterator {
        friend class List;
    private:
        BaseNode* ptr;

    public:
        using value_type = conditional<IsConst, const T, T>;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        using pointer = conditional_t<IsConst, const T*, T*>;
        using reference = conditional_t<IsConst, const T&, T&>;

        explicit common_iterator(const BaseNode* ptr) : ptr(const_cast<BaseNode*>(ptr)) {}

        common_iterator(const common_iterator<false>& copy)
                : ptr(copy.ptr) {}

        common_iterator& operator=(const common_iterator& other) {
            common_iterator copy(other);
            swap(copy);
            return *this;
        }

        void swap(common_iterator& copy) {
            std::swap(ptr, copy.ptr);
        }

        common_iterator& operator++() {
            ptr = ptr->next;
            return *this;
        }

        common_iterator& operator--() {
            ptr = ptr->prev;
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

        bool operator==(const common_iterator<IsConst>& other) const {
            return ptr == other.ptr;
        }

        bool operator!=(const common_iterator<IsConst>& other) const {
            return ptr != other.ptr;
        }
        reference operator*() const {
            return static_cast<Node*>(ptr)->value_;
        }

        pointer operator->() const {
            return &static_cast<Node*>(ptr)->value_;
        }

        operator common_iterator<true>() {
            return common_iterator<true>(ptr);
        }
    };

    BaseNode head_ = BaseNode(&head_, &head_);
    size_t size_ = 0;
    Alloc alloc_;

    void swap(List& other) {
        auto this_prev = head_.prev;
        auto other_prev = other.head_.prev;
        auto this_next = head_.next;
        auto other_next = other.head_.next;
        std::swap(other_prev->next, this_prev->next);
        std::swap(other_next->prev, this_next->prev);
        std::swap(head_, other.head_);
        std::swap(size_, other.size_);
        std::swap(alloc_, other.alloc_);
    }
public:
    using iterator = common_iterator<false>;
    using const_iterator = common_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using NodeAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
    iterator begin() {
        return iterator(head_.next);
    }

    iterator end() { return iterator(&head_); }

    const_iterator cbegin() const {
        return const_iterator(head_.next);
    }
    const_iterator begin() const {
        return const_iterator(head_.next);
    }

    const_iterator cend() const {
        return const_iterator(&head_);
    }

    const_iterator end() const {
        return const_iterator(&head_);
    }

    reverse_iterator rbegin() {
        return std::make_reverse_iterator(end());
    }

    reverse_iterator rend() {
        return std::make_reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const {
        return std::make_reverse_iterator(cend());
    }

    const_reverse_iterator rbegin() const {
        return std::make_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const {
        return std::make_reverse_iterator(cbegin());
    }

    const_reverse_iterator rend() const {
        return std::make_reverse_iterator(cbegin());
    }

    explicit List(const Alloc& alloc = Alloc()): alloc_(alloc) {}

    List(size_t num, const T& value, const Alloc& alloc = Alloc()): alloc_(alloc) {
        try {
            for (size_t i = 0; i < num; ++i) {
                emplace_back(value);
            }
        } catch(...) {
            clear_list();
            throw;
        }
    }

    explicit List(size_t num, const Alloc& alloc = Alloc()) : List(alloc) {
        try {
            for (size_t i = 0; i < num; ++i) {
                emplace_back();
            }
        } catch(...) {
            clear_list();
            throw;
        }
    }

    List (const List& other) : alloc_(std::allocator_traits<Alloc>::select_on_container_copy_construction(other.alloc_)) {
        try {
            for (const auto& el:other) {
                push_back(el);
            }
        } catch (...) {
            clear_list();
            throw;
        }
    }

    List& operator=(const List& other) {
        Alloc alloc = std::allocator_traits<Alloc>::propagate_on_container_copy_assignment::value ? other.alloc_ : alloc_;
        List<T, Alloc> copy(alloc);
        Node* ptr = static_cast<Node*>(other.head_.next);
        for (size_t i = 0; i < other.size_; ++i) {
            copy.push_back(ptr->value_);
            if (i + 1 != other.size_) {
                ptr = static_cast<Node*>(ptr->next);
            }
        }
        swap(copy);
        return *this;
    }

    template<typename ...Args>
    void emplace_back(Args&&... args) {
        emplace(end(), std::forward<Args>(args)...);
    }

    template<typename ...Args>
    void emplace(const_iterator iter, Args&&... args) {
        NodeAlloc nalloc(alloc_);
        Node* newnode = std::allocator_traits<NodeAlloc>::allocate(nalloc, 1);
        try {
            std::allocator_traits<NodeAlloc>::construct(nalloc, newnode, std::forward<Args>(args)...);
        } catch(...) {
            std::allocator_traits<NodeAlloc>::deallocate(nalloc, newnode, 1);
            throw;
        }
        newnode->next = iter.ptr;
        newnode->prev = iter.ptr->prev;
        iter.ptr->prev->next = newnode;
        iter.ptr->prev = newnode;
        ++size_;
    }


    void push_back(const T& value) {
        emplace_back(value);
    }

    void push_front(const T& value) {
        emplace(begin(), value);
    }

    void pop_back() {
        erase(--end());
    }

    void pop_front() {
        erase(begin());
    }

    void insert(const_iterator iter, const T& value) {
        emplace(iter, value);
    }

    void erase(const_iterator iter) {
        NodeAlloc nalloc(alloc_);
        Node* todelete = static_cast<Node*>(iter.ptr);
        todelete->prev->next = todelete->next;
        todelete->next->prev = todelete->prev;
        std::allocator_traits<NodeAlloc>::destroy(nalloc, todelete);
        std::allocator_traits<NodeAlloc>::deallocate(nalloc, todelete, 1);
        --size_;
    }

    size_t size() const {
        return size_;
    }

    Alloc get_allocator() {
        return alloc_;
    }

    void clear_list() {
        while (size_) {
            pop_back();
        }
    }

    ~List() {
        clear_list();
    }
};
