#include <functional>
#include <iostream>
#include <type_traits>
#include <memory>

template<class T, class vtable_t, std::size_t _size = 24, std::size_t _alignment = 8>
class local_buffer_t {
    static constexpr std::size_t buffer_size = _size - sizeof(T*);
    static constexpr std::ptrdiff_t one = 1;
    static constexpr std::ptrdiff_t not_one = ~one;

    T* _ptr;
    alignas(_alignment) std::byte _buffer[buffer_size];

    template<class Deleter>
    struct state_0_t  {
        union {
            [[no_unique_address]] Deleter d;
            const std::byte padding[buffer_size - sizeof(vtable_t*)];
        };
        const vtable_t* vtable;
    };
    template<class Deleter>
    struct state_1_t {
        union {
            [[no_unique_address]] Deleter d;
            const std::byte padding[buffer_size - sizeof(vtable_t)];
        };
        const vtable_t vtable;
    };

    template<std::size_t state>
    void set_state() noexcept {
        if constexpr (state == 0) {
            _ptr = reinterpret_cast<T*>(reinterpret_cast<std::ptrdiff_t>(_ptr) & not_one);
        }
        else {
            _ptr = reinterpret_cast<T*>(reinterpret_cast<std::ptrdiff_t>(_ptr) | one);
        }
    }
     auto get_state() const noexcept {
        return static_cast<std::size_t>(reinterpret_cast<std::ptrdiff_t>(_ptr) & one);
    }
public:
    template<class Deleter>
    local_buffer_t(T* _data, Deleter&& deleter, const vtable_t* vtable) : _ptr{_data} {
        static_assert(alignof(T) > 1);
        if constexpr(sizeof(state_1_t<Deleter>) == _size - sizeof(T*)) {
            new (static_cast<void*>(_buffer)) state_1_t<Deleter>({std::forward<Deleter>(deleter)},  *vtable);
            set_state<1>();
            static_assert(offsetof(state_1_t<Deleter>, vtable) == buffer_size - sizeof(vtable_t));
        }
        else {
            static_assert(sizeof(state_0_t<Deleter>) <= buffer_size);
            new (static_cast<void*>(_buffer)) state_0_t<Deleter>({std::forward<Deleter>(deleter)}, vtable);
            set_state<0>();
            static_assert(offsetof(state_0_t<Deleter>, vtable) == buffer_size - sizeof(vtable_t*));
        }
    }
    T* get() const noexcept {
        return reinterpret_cast<T*>(reinterpret_cast<std::ptrdiff_t>(_ptr) & not_one);
    }
    const vtable_t& vtable() noexcept {
        if(get_state() == 1) {
            return static_cast<state_1_t<std::default_delete<T>>*>(static_cast<void*>(static_cast<std::byte*>(_buffer)))->vtable;
        }
        return *(static_cast<state_0_t<std::default_delete<T>>*>(static_cast<void*>(static_cast<std::byte*>(_buffer)))->vtable);
    }
    void* deleter() noexcept {
        return reinterpret_cast<void*>(_buffer);
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
            static_cast<Deleter*>(deleter)->~Deleter();
            //delete static_cast<Deleter*>(deleter);
        }
        static constexpr vtable_t vtable{destroy, destructor};
    };
    template<class Deleter>
    static constexpr vtable_t vtable = vtable_impl<std::remove_reference_t<Deleter>>::vtable;

    /*T* _ptr;
    void* _deleter;
    const vtable_t* _vtable;*/

    local_buffer_t<T, vtable_t> _buffer;
    T* get() const noexcept {
        return _buffer.get();
    }
    const vtable_t& _vtable() noexcept {
        return _buffer.vtable();
    }
    void* _deleter() noexcept {
        return _buffer.deleter();
    }

public:
    template<class Deleter>
    smartptr_te(T* _ptr, Deleter&& _deleter) :
        _buffer (_ptr, std::forward<Deleter>(_deleter), &vtable<Deleter>) {
        static_assert(std::is_nothrow_move_constructible_v<Deleter>);
    }
    ~smartptr_te() {
        _vtable()._destroy(get(), _deleter());
        _vtable()._destructor(_deleter());
    }
    template<class Deleter>
    void set_deleter() {
        static_assert(std::is_nothrow_move_constructible_v<Deleter>);
        _vtable->_destructor(_deleter);
        _deleter = new Deleter{std::forward<Deleter>(_deleter)};
        _vtable = &vtable<Deleter>;
    }
    auto& operator=(const smartptr_te& rhs) noexcept {

    }
    auto& operator=(smartptr_te&& rhs) noexcept {

    }
};

int main() {
    /*auto my_deleter = [](void* ptr) {
        delete static_cast<int*>(ptr);
        std::cout << "destroy called!";
    };*/
    struct my_deleter : decltype([](void* ptr) {
        delete static_cast<int*>(ptr);
        std::cout << "destroy called!";
    }) {};

    std::cout << std::is_empty_v<my_deleter> << std::endl;

    smartptr_te p1(new int{42}, std::default_delete<int>{});
    smartptr_te p2(new int{42}, my_deleter{});
    
    return 0;
}
