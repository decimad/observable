#include "observable.h"
#include <iostream>

class my_model;

class MyModelObserver {
public:
    virtual void OnMyModelChanged(my_model* who) = 0;
};

class my_model : public util::observable< MyModelObserver >
{
public:
    my_model()
    {}

    my_model(util::observable< MyModelObserver >& inherited)
        : observable(inherited)
    {}

    void changed() {
        notify(&MyModelObserver::OnMyModelChanged, this);
    }
};

class my_observer : public MyModelObserver
{
public:
    my_observer(util::observable<MyModelObserver>& other)
    {
        link_ = other.subscribe(this);
    }

    void OnMyModelChanged(my_model* who) override {
        std::cout << "A model changed: " << who << "\n";
        link_.reset();  // can unlink during iteration
    }

private:
    util::link link_;
};

int main()
{
    util::observable< MyModelObserver > shared;

    {
        my_model model1(shared);
        my_model model2;
        my_observer obs1(model1);
        my_observer obs2(model2);

        shared.notify(&MyModelObserver::OnMyModelChanged, nullptr);
        model1.changed();
        model2.changed();
    }

    std::cout << "returning from main\n";

    return 0;
}