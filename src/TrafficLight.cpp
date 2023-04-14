#include "TrafficLight.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>

/* Implementation of class "MessageQueue" */

template <typename T> T MessageQueue<T>::receive() {
  std::unique_lock<std::mutex> uLock(
      _mutex); // modify queue under lock (using unique instead of lock guard
               // for flexability)
  _cond.wait(uLock, [this] {
    return !_messages.empty();
  }); // pass unique lock to condition variable

  // returning the last element in queue and removing it
  T msg = std::move(_messages.back());
  _messages.pop_back(); // will not be copied due to return value optimization
                        // (RVO) in C++

  return msg;
}

template <typename T> void MessageQueue<T>::send(T &&msg) {
  std::lock_guard<std::mutex> gLock(
      _mutex); // modify queue under lock to stop data races
  _messages.clear();
  _messages.push_back(std::move(msg));
  _cond.notify_one(); // send notification to sleep client to add new message to
                      // queue
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight() : _currentPhase(TrafficLightPhase::red) {
    _currentPhase = TrafficLightPhase::red;
  msgQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
} // initialize traffic light with red

TrafficLightPhase TrafficLight::getCurrentPhase() {
  return _currentPhase;
} // return the current phase of traffic light

void TrafficLight::simulate() // initializing new threads in threads vector
                              // using cycleThroughPhases function
{
  threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

void TrafficLight::cycleThroughPhases() {

  // Seed the random number generator with the current time
  srand(time(nullptr));

  auto lastUpdate = std::chrono::system_clock::now(); // time now
 int rand_num = rand(); // Generate a random number between 0 and RAND_MAX
 // Scale the random number to the range of 4000 to 6000
     // 2001 is the range between 0 and 2000 inclusive
 int duration =
        4000 +
        rand_num % 2001;
  while (true) {
    long timeSinceLastUpdate =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - lastUpdate)
            .count();
    if (timeSinceLastUpdate >= duration) {
      _currentPhase = _currentPhase == TrafficLightPhase::red
                          ? TrafficLightPhase::green
                          : TrafficLightPhase::red; // changing phase

      // moving message to queue
    //   auto msg = _currentPhase;
      msgQueue->send(std::move(_currentPhase));

      lastUpdate = std::chrono::system_clock::now(); // setting time again to
                                                     // the time now
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void TrafficLight::waitForGreen() {

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (msgQueue->receive() == TrafficLightPhase::green)
      return;
  }
}
