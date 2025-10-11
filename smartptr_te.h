//
// Created by Herman on 10/8/2025.
//

#ifndef SMARTPTR_TE_H
#define SMARTPTR_TE_H

#endif //SMARTPTR_TE_H
#include <functional>
#include <iostream>
#include <type_traits>
#include <memory>

namespace lbo_optimized {
    template<class T, std::size_t _size = 24, std::size_t _alignment = 8>
    class local_buffer_t {
        static constexpr std::size_t buffer_size = _size - sizeof(T*);
        static constexpr std::ptrdiff_t one = 1;
        static constexpr std::ptrdiff_t not_one = ~one;

    public:
        struct vtable_t {
            std::add_pointer_t<void(T*, void*)> _destroy;
            std::add_pointer_t<void(void*)> _destructor;
        };
    private:

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
        static constexpr vtable_t vtable_v = vtable_impl<std::remove_reference_t<Deleter>>::vtable;

        T* _ptr;
        alignas(_alignment) std::byte _buffer[buffer_size];

        template<class Deleter>
        struct temp0 {
            [[no_unique_address]] std::remove_reference_t<Deleter> d;
            const vtable_t* vtable;
        };
        template<class Deleter>
        struct temp1 {
            [[no_unique_address]] std::remove_reference_t<Deleter> d;
            const vtable_t vtable;
        };

        template<class Deleter>
        struct state_0_t  {
            [[no_unique_address]] std::remove_reference_t<Deleter> d;
            const std::byte padding[buffer_size - sizeof(temp0<Deleter>)];
            const vtable_t* vtable;
        };
        template<class Deleter>
        struct state_1_t {
            [[no_unique_address]] std::remove_reference_t<Deleter> d;
            const std::byte padding[buffer_size - sizeof(temp1<Deleter>)];
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
        local_buffer_t(T* _ptr, Deleter&& deleter) : _ptr{_ptr} {
            static_assert(alignof(T) > 1);
            if constexpr(sizeof(state_1_t<Deleter>) == _size - sizeof(T*)) {
                new (static_cast<void*>(_buffer)) state_1_t<Deleter>(std::forward<Deleter>(deleter), {},  vtable_v<Deleter>);

                set_state<1>();
                static_assert(offsetof(state_1_t<Deleter>, vtable) == buffer_size - sizeof(vtable_t));
            }
            else {
                //std::cout << typeid(deleter).name() << ' ' << std::is_empty_v<Deleter> << ' ' << sizeof(state_1_t<Deleter>) << std::endl;
                static_assert(sizeof(state_0_t<Deleter>) <= buffer_size);
                new (static_cast<void*>(_buffer)) state_0_t<Deleter>(std::forward<Deleter>(deleter), {}, &vtable_v<Deleter>);
                set_state<0>();
                static_assert(offsetof(state_0_t<Deleter>, vtable) == buffer_size - sizeof(vtable_t*));
            }
        }
        T* get() const noexcept {
            return reinterpret_cast<T*>(reinterpret_cast<std::ptrdiff_t>(_ptr) & not_one);
        }
        const vtable_t& vtable() noexcept {
            const auto state = get_state();
            const auto* vtable_0 = static_cast<state_0_t<std::default_delete<T>>*>(static_cast<void*>(static_cast<std::byte*>(_buffer)))->vtable;
            const auto* vtable_1 = &static_cast<state_1_t<std::default_delete<T>>*>(static_cast<void*>(static_cast<std::byte*>(_buffer)))->vtable;
            return *reinterpret_cast<vtable_t*>((state == 0) * reinterpret_cast<std::ptrdiff_t>(vtable_0) + state * reinterpret_cast<std::ptrdiff_t>(vtable_1));

            if(get_state()) {
                return static_cast<state_1_t<std::default_delete<T>>*>(static_cast<void*>(static_cast<std::byte*>(_buffer)))->vtable;
            }
            return *(static_cast<state_0_t<std::default_delete<T>>*>(static_cast<void*>(static_cast<std::byte*>(_buffer)))->vtable);
        }
        void* deleter() noexcept {
            return reinterpret_cast<void*>(_buffer);
        }
        template<class Deleter>
        void set_deleter(Deleter deleter) noexcept {
            if constexpr(sizeof(state_1_t<Deleter>) == _size - sizeof(T*)) {
                new (static_cast<void*>(_buffer)) state_1_t<Deleter>(std::forward<Deleter>(deleter), {},  vtable_v<Deleter>);
                set_state<1>();
                static_assert(offsetof(state_1_t<Deleter>, vtable) == buffer_size - sizeof(vtable_t));
            }
            else {
                static_assert(sizeof(state_0_t<Deleter>) <= buffer_size);
                new (static_cast<void*>(_buffer)) state_0_t<Deleter>(std::forward<Deleter>(deleter), {}, &vtable_v<Deleter>);
                set_state<0>();
                static_assert(offsetof(state_0_t<Deleter>, vtable) == buffer_size - sizeof(vtable_t*));
            }
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

        local_buffer_t<T> _buffer;
        using vtable_t = typename decltype(_buffer)::vtable_t;

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
            _buffer (_ptr, std::forward<Deleter>(_deleter)) {
            static_assert(std::is_nothrow_move_constructible_v<Deleter>);
        }

        ~smartptr_te() {
            const auto& vtable = _vtable();
            vtable._destroy(get(), _deleter());
            vtable._destructor(_deleter());
        }
        template<class Deleter>
        void set_deleter(Deleter&& deleter) {
            static_assert(std::is_nothrow_move_constructible_v<Deleter>);
            _vtable()._destructor(_deleter());
            _buffer.set_deleter(std::forward<Deleter>(deleter));
        }
    };

}

namespace lbo_original {
    template<std::size_t _size = 16, std::size_t _alignment = 8>
    class local_buffer_t {
        alignas(_alignment) std::byte _buffer[_size];
    public:
        template<class Deleter>
        local_buffer_t(Deleter&& deleter) {
            new (static_cast<void*>(_buffer)) Deleter{std::forward<Deleter>(deleter)};
            static_assert(sizeof(Deleter) <= _size);
            static_assert(std::is_nothrow_move_constructible_v<Deleter>);
        }

        void* get() noexcept {
            return reinterpret_cast<void*>(_buffer);
        }

        template<class Deleter>
        void set(Deleter&& deleter) noexcept {
            new (static_cast<void*>(_buffer)) Deleter{std::forward<Deleter>(deleter)};
        }
    };


    template<class T>
    class smartptr_te {
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
            }
            static constexpr vtable_t vtable{destroy, destructor};
        };
        template<class Deleter>
        static constexpr vtable_t vtable = vtable_impl<std::remove_reference_t<Deleter>>::vtable;

        T* _ptr;
        const vtable_t* _vtable;
        local_buffer_t<> _buffer;

        void* _deleter() noexcept {
            return _buffer.get();
        }

    public:
        template<class Deleter>
        smartptr_te(T* _ptr, Deleter&& _deleter) :
        _ptr(_ptr), _vtable(&vtable<Deleter>), _buffer(std::forward<Deleter>(_deleter)) {
            static_assert(std::is_nothrow_move_constructible_v<Deleter>);
        }

        ~smartptr_te() {
            _vtable->_destroy(_ptr, _deleter());
            _vtable->_destructor(_deleter());
        }
        template<class Deleter>
        void set_deleter(Deleter&& deleter) {
            static_assert(std::is_nothrow_move_constructible_v<Deleter>);
            _vtable->_destructor(_deleter());
            _buffer.set(std::forward<Deleter>(deleter));
            _vtable = &vtable<Deleter>;
        }
    };
}
