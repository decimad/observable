#include "observable.h"

namespace util {

    namespace detail {

        template< typename T >
        struct equals {
            equals(const T& val) : val_(val) {}

            bool operator()(const T& val) {
                return val == val_;
            }

        private:
            const T& val_;
        };

        observable_base::impl::impl(observable_base* owned_by) : owned_by_(owned_by), iteration_count_(0)
        {}

        bool observable_base::impl::is_shared() const {
            return owned_by_ == nullptr;
        }

        void observable_base::impl::set_shared() {
            owned_by_ = nullptr;
        }

        observable_base::impl::~impl()
        {
            std::cout << "impl::~impl() called\n";
        }

        void observable_base::impl::remove_observer(void * observer)
        {
            if(iteration_count_) {
                for(auto*& ptr : observers_) {
                    if(ptr == observer) {
                        ptr = nullptr;
                    }
                }
            }
            else {
                collect();
                if(observers_.empty() && owned_by_) {
                    owned_by_->on_observers_empty();
                }
            }
        }

        void observable_base::impl::collect()
        {
            auto it = std::remove_if(observers_.begin(), observers_.end(), detail::equals<void*>(nullptr));
            observers_.erase(it, observers_.end());
        }
    }

    link::link()
    {
    }

    link::link(std::shared_ptr<detail::observable_base::impl> token, void* observer)
        : token_(std::move(token)), observer_(observer)
#ifdef _DEBUG
        , moved_away_(false)
#endif
    {
    }

    link::link(link&& other)
        : token_(std::move(other.token_)), observer_(other.observer_)
    {
        other.token_.reset();
#ifdef _DEBUG
        other.moved_away_ = true;
#endif
    }

    link& link::operator=(link&& other) {
        token_ = std::move(other.token_);
        other.token_.reset();
        observer_ = other.observer_;
#ifdef _DEBUG
        other.moved_away_ = true;
#endif
        return *this;
    }

    link::operator bool() const {
        return token_ ? true : false;
    }

    link::~link()
    {
#ifdef _DEBUG
        assert(moved_away_ && "You forgot to save the observer connection");
#endif
        reset();
    }

    void link::reset()
    {
        if(token_) {
            token_->remove_observer(observer_);
        }
    }

    namespace detail {

        observable_base::observable_base()
        {
        }

        observable_base::observable_base(observable_base& other)
            : impl_(other.get_shared_impl())
        {
        }

        observable_base::~observable_base()
        {
        }

        size_t observable_base::size() const {
            if(impl_) {
                return impl_->observers_.size();
            }
            else {
                return 0;
            }
        }

        void observable_base::remove_observer(void* observer) {
            assert(impl_);
            impl_->remove_observer(observer);
        }

        void observable_base::allocate_impl() {
            if(!impl_) {
                impl_ = std::make_shared<impl>(this);
            }
        }

        void observable_base::allocate_shared_impl()
        {
            if(!impl_) {
                impl_ = std::make_shared<impl>();
            }
            else {
                impl_->set_shared();
            }
        }

        void observable_base::deallocate_impl() {
            impl_.reset();
        }

        void observable_base::enter_iteration() {
            if(impl_) {
                ++impl_->iteration_count_;
            }
        }

        void observable_base::leave_iteration() {
            if(impl_) {
                --impl_->iteration_count_;
                if(impl_->iteration_count_ == 0) {
                    if(!impl_->is_shared() && impl_.use_count() == 1) {
                        impl_.reset();
                    }
                    else {
                        impl_->collect();
                    }
                }
            }
        }

        void observable_base::on_observers_empty()
        {
            // called by an impl that is owned by this, ie. it is a non-shared impl
            // also it is not iterating
            deallocate_impl();
        }

        std::shared_ptr<observable_base::impl> observable_base::get_shared_impl()
        {
            allocate_shared_impl();
            return impl_;
        }

        void observable_base::add_observer(void* observer) {
            allocate_impl();
            impl_->observers_.push_back(observer);
        }

    }


}