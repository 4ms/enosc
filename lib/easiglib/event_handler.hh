#include "buffer.hh"

template<class Event>
struct EventSource : Nocopy {
  virtual void Poll(std::function<void(Event)> put) = 0;
};

template<class T, class Event>
struct EventHandler : crtp<T, EventHandler<T, Event>> {

  using EventStack = BufferReader<Event, 8>;

  void Poll() {
    for(auto& s : (**this).sources_)
      s->Poll([this](Event e){
               events_.Write(e);
               pending++;
             });
  }

  void Process() {
    while(pending >= 0) {
      (**this).Handle(EventStack(events_, pending));
      pending--;
    }
  }

  struct DelayedEventSource : EventSource<Event> {
    int count_ = -1;
    Event event_;
    void Poll(std::function<void(Event)> put) {
      if (count_ >= 0 && count_-- == 0) put(event_);
    }
    void trigger_after(int delay, Event e) {
      event_ = e;
      count_ = delay;
    }
  };
  
private:
  RingBuffer<Event, 8> events_;
  // number of events remaining to handle - 1
  int pending = -1;
};
