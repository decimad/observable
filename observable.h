#pragma once
#include <cassert>
#include <memory>
#include <vector>
#include <algorithm>
#include <functional>
#include <iostream>

/*
model subscriber implementation using links to represent connections.
subscribers can be removed during notification.
if no observers are registered, size is the size of a shared_ptr.
*/

namespace util {

    class link;

    namespace detail {

        struct observable_base {
        public:
            observable_base();
            observable_base(observable_base& other);

            ~observable_base();

            size_t size() const;

        private:
            friend class link;

            struct impl {
                impl(observable_base* owned_by = nullptr);
                bool is_shared() const;
                void set_shared();
                ~impl();

                void remove_observer(void* observer);
                void collect();

                std::vector<void*> observers_;
                unsigned int iteration_count_;
                observable_base* owned_by_;
            };

            void allocate_impl();
            void allocate_shared_impl();

            void deallocate_impl();

            std::shared_ptr<impl> get_shared_impl();

        protected:
            void on_observers_empty();

            void enter_iteration();
            void leave_iteration();

            void add_observer(void* observer);
            void remove_observer(void* observer);

            std::shared_ptr<impl> impl_;
        };

    }

    class link {
    public:
        link();
        link(std::shared_ptr<detail::observable_base::impl> token, void* observer);
        link(link&& other);
        link(const link&) = delete;

        void operator=(const link&) = delete;
        link& operator=(link&& other);

        void reset();

        explicit operator bool() const;

        ~link();

    private:
#ifdef _DEBUG
        bool moved_away_ = true;
#endif
        std::shared_ptr<detail::observable_base::impl> token_;
        void* observer_;
    };


    template< typename ObserverInterface >
    class observable :
        public detail::observable_base
    {
    public:
        observable() {}

        observable(observable& inherit)
            : observable_base(inherit)
        {}

        void subscribe_unmanaged(ObserverInterface* obs)
        {
            return observable_base::add_observer_unmanaged(static_cast<void*>(obs));
        }

        link subscribe(ObserverInterface* obs) {
            observable_base::add_observer(static_cast<void*>(obs));
            return link(impl_, static_cast<void*>(obs));
        }

        // Must only be called for observers subscribed with subscribe_unmanaged
        void unsubscribe(ObserverInterface* obs)
        {
            observable_base::remove_observer(static_cast<void*>(obs));
        }

        template< typename... Args1, typename... Args2 >
        void notify(void(ObserverInterface::*Ptr)(Args1...), Args2&&... args)
        {
            static_assert(sizeof...(Args1) == sizeof...(Args2), "argument counts don't match");

            if(impl_) {
                enter_iteration();
                try {
                    for(auto* void_ptr : impl_->observers_) {
                        if(void_ptr) {
                            auto* typed_ptr = static_cast<ObserverInterface*>(void_ptr);
                            (typed_ptr->*Ptr)(std::forward<Args2>(args)...);
                        }
                    }
                }
                catch(...) {
                    leave_iteration();
                    throw;
                }

                leave_iteration();
            }
        }

        template< typename... Args1, typename... Args2 >
        void notify_ignore(void(ObserverInterface::*Ptr)(Args1...), ObserverInterface* ignore, Args2&&... args)
        {
            static_assert(sizeof...(Args1) == sizeof...(Args2), "argument counts don't match");

            if(impl_) {
                enter_iteration();
                try {
                    for(auto* void_ptr : impl_->observers_) {
                        if(void_ptr && void_ptr != ignore) {
                            auto* typed_ptr = static_cast<ObserverInterface*>(void_ptr);
                            (typed_ptr->*Ptr)(std::forward<Args2>(args)...);
                        }
                    }
                }
                catch(...) {
                    leave_iteration();
                    throw;
                }
                leave_iteration();
            }
        }

    };

    template< typename Class >
    struct subscriber_mixin {

        template< typename Model >
        void subscribe(Model& model) {
            links_.emplace_back(model.subscribe(static_cast<Class*>(this)));
        }

        void clear_links() {
            links_.clear();
        }

    private:
        std::vector< link > links_;
    };

}