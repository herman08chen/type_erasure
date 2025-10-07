#include <iostream>
#include <type_traits>
#include <memory>


template<std::size_t N, std::size_t alignment = 8>
class buffer {
    alignas(alignment) std::array<std::byte, N> _data;
public:
    template<class T>
    buffer(const T& d) {
        static_assert(sizeof(T) <= N);
        new (_data.data()) T{d};
    }
    template<class T>
    buffer(T&& d) {
        static_assert(sizeof(T) <= N);
        new (_data.data()) T{std::move(d)};
    }
    template<class T>
    T& get() const noexcept {
        return *static_cast<T*>(_data.data());
    }
};

template<class T>
concept Stateless = std::is_empty_v<T>;

template<class T, class vtable_t, std::size_t _size = 24, std::size_t _alignment = 8>
class local_buffer_t {
    static constexpr std::size_t buffer_size = _size - sizeof(T*);
    T* _data;
    alignas(_alignment) std::byte _buffer[buffer_size];

    template<Stateless Deleter>
    struct state_0_t  {
        [[no_unique_address]] Deleter d;
        std::byte padding[buffer_size - sizeof(vtable_t*)];
        vtable_t* vtable;
    };
    template<class Deleter>
    struct state_0_t  {
        Deleter d;
        std::byte padding[buffer_size - sizeof(vtable_t*) - sizeof(Deleter)];
        vtable_t* vtable;
    };
    template<Stateless Deleter>
    struct state_1_t  {
        [[no_unique_address]] Deleter d;
        std::byte padding[buffer_size - sizeof(vtable_t*)];
        vtable_t vtable;
    };
    template<class Deleter>
    struct state_1_t  {
        Deleter d;
        std::byte padding[buffer_size - sizeof(vtable_t*) - sizeof(Deleter)];
        vtable_t vtable;
    };

    static constexpr std::ptrdiff_t one = 1;
    static constexpr std::ptrdiff_t not_one = ~one;

    template<class Deleter>
    local_buffer_t(T* _data, Deleter&& deleter, const vtable_t* vtable) : _data{_data} {
        static_assert(alignof(T) > 1);
        if constexpr(sizeof(state_1_t<Deleter>) <= _size - sizeof(T*)) {
            new (static_cast<void*>(_buffer)) state_1_t<Deleter>(std::forward<Deleter>(deleter), vtable);
            set_state<1>();
        }
        else {
            new (static_cast<void*>(_buffer)) state_0_t<Deleter>(std::forward<Deleter>(deleter), *vtable);
            set_state<0>();
        }
    }
    template<std::size_t state>
    void set_state() noexcept {
        if constexpr (state == 0) {
            _data = static_cast<T*>(static_cast<std::ptrdiff_t>(_data) & not_one);
        }
        else {
            _data = static_cast<T*>(static_cast<std::ptrdiff_t>(_data) | one);
        }
    }
     auto get_state() const noexcept {
        return static_cast<std::ptrdiff_t>(_data & one);
    }
public:
    T* get() const noexcept {
        return static_cast<T*>(static_cast<std::ptrdiff_t>(_data) & not_one);
    }
    const vtable_t& get_vtable() const noexcept {
        if(get_state == 1) {
            return static_cast<state_1_t<std::default_delete<T>>*>(_buffer)->vtable;
        }
        return *(static_cast<state_1_t<std::default_delete<T>>*>(_buffer)->vtable);
    }
    void* get_deleter() const noexcept {
        return static_cast<void*>(_buffer);
    }
};

template<class T>
class smartptr_te {
    /*template<class Deleter>
    static void destroy(T* ptr, void* deleter) {
        (*static_cast<Deleter*>(deleter))(ptr);
    }
    template<class Deleter>
    static void destructor(void* deleter) {
        //(*static_cast<Deleter*>(deleter))->~Deleter();
        delete static_cast<Deleter*>(deleter);
    }
    struct vtable_t {
        std::add_pointer_t<void(T*, void*)> _destroy;
        std::add_pointer_t<void(void*)> _destructor;
    };
    template<class Deleter>
    static constexpr vtable_t vtable = {
        destroy<Deleter>,
        destructor<Deleter>
    };*/

    /*struct {
        template<class Deleter>
        struct vtable_implementation {
                const std::add_pointer_t<void(T*, void*)> _destroy = +[](T* ptr, void* deleter) -> void {
                    (*static_cast<Deleter*>(deleter))(ptr);
                };
                const std::add_pointer_t<void(void*)> _destructor = +[](void* deleter) -> void {
                    //(*static_cast<Deleter*>(deleter))->~Deleter();
                    delete static_cast<Deleter*>(deleter);
                };
            };
        using vtable_t = vtable_implementation<void>;
        template<class Deleter>
        static constexpr auto static_vtables = vtable_implementation<Deleter>{};

        template<class Deleter>
        inline static const vtable_t& vtable = reinterpret_cast<const vtable_t&>(static_vtables<Deleter>);
    } ;*/

    struct vtable_t {
        std::add_pointer_t<void(T*, void*)> _destroy;
        std::add_pointer_t<void(void*)> _destructor;
    };

    template<class Deleter>
    struct vtable_impl {
        static void destroy(T* ptr, void* deleter) {
            (*static_cast<Deleter*>(deleter))(ptr);
        }
        static void destructor(void* deleter) {
            //(*static_cast<Deleter*>(deleter))->~Deleter();
            delete static_cast<Deleter*>(deleter);
        }
        static constexpr vtable_t vtable{destroy, destructor};
    };
    template<class Deleter>
    static constexpr vtable_t vtable = vtable_impl<Deleter>::vtable;

    T* _ptr;
    void* _deleter;
    const vtable_t* _vtable;
public:
    template<class Deleter>
    smartptr_te(T* _ptr, Deleter _deleter) :
        _ptr(_ptr), _deleter(new Deleter{std::forward<Deleter>(_deleter)}), _vtable(&vtable<Deleter>) {
        static_assert(std::is_nothrow_move_constructible_v<Deleter>);
    }
    ~smartptr_te() {
        _vtable->_destroy(_ptr, _deleter);
        _vtable->_destructor(_deleter);
    }
    template<class Deleter>
    void set_deleter() {
        static_assert(std::is_nothrow_move_constructible_v<Deleter>);
        _vtable->_destructor(_deleter);
        _deleter = new Deleter{std::forward<Deleter>(_deleter)};
        _vtable = &vtable<Deleter>;
    }
    auto& operator=(const smartptr_te& rhs) noexcept {
        if (this != &rhs) {
            _ptr = rhs._ptr;
        }
    }
    auto& operator=(smartptr_te&& rhs) noexcept {
        std::swap(_ptr, rhs._ptr);
        std::swap(_deleter, rhs._deleter);
        std::swap(_vtable, rhs._vtable);
    }
};

int main() {
    auto my_deleter = [](void* ptr) {
        delete static_cast<int*>(ptr);
        std::cout << "destroy called!";
    };
    std::cout << sizeof(std::add_pointer_t<void(int*, void*)>) << std::endl;
    std::cout << sizeof(std::add_pointer_t<void(void*)>) << std::endl;

    smartptr_te p1(new int{42}, std::default_delete<int>{});
    smartptr_te p2(new int{42}, my_deleter);
    
    return 0;
}
