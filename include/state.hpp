#ifndef STATE_HPP
#define STATE_HPP

#include <vector>
#include <memory>

class State
{
public:
    State() :
        has_started_(false) {}

    virtual ~State() {}

    // Executed immediately when the state is pushed.
    virtual void on_pushed() {}

    // Executed the first time that the state becomes the current state.
    // Sometimes multiple states may be pushed in a sequence, and one of the
    // later states may want to perform actions only when it actually becomes
    // the current state.
    virtual void on_start() {}

    // Executed immediately when the state is popped.
    // This should only be used for cleanup, do not push or pop other states
    // from this call (this is not supported).
    virtual void on_popped() {}

    virtual void draw() {}

    // If true, this state is drawn overlayed on the state(s) below. This can be
    // used for example to draw a marker state on top of the map.
    virtual bool draw_overlayed() const
    {
        return false;
    }

    // Read input, process game logic etc.
    virtual void update() {}

    // All states above have been popped
    virtual void on_resume() {}

    // Another state is pushed on top
    virtual void on_pause() {}

    bool has_started() const
    {
        return has_started_;
    }

    void set_started()
    {
        has_started_ = true;
    }

    bool has_started()
    {
        return has_started_;
    }

private:
    bool has_started_;
};

namespace states
{

void init();

void cleanup();

void start();

void draw();

void update();

void push(std::unique_ptr<State> state);

void pop();

void pop_all();

bool is_empty();

bool is_current_state(const State& state);

} // states

#endif // STATE_HPP
