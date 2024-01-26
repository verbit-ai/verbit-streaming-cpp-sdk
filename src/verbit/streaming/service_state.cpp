#include <stdexcept>

#include "service_state.h"

namespace verbit {
namespace streaming {

const char* const ServiceState::c_str()
{
	switch (_state) {
	case state_initial:
		return "initial";
	case state_opening:
		return "opening";
	case state_open:
		return "open";
	case state_closing:
		return "closing";
	case state_done:
		return "done";
	case state_fail:
		return "fail";
	default:
		return "?";
	}
}

void ServiceState::change(int state)
{
	std::unique_lock<std::mutex> mlock(_state_mutex);
	_state = state;
	_state_changed.notify_one();
}

void ServiceState::change_if(int state, int expected_state, bool throw_ex)
{
	std::unique_lock<std::mutex> mlock(_state_mutex);
	if ((_state != expected_state) && throw_ex) {
		std::string error = std::string("change_if failed changing state=") + std::to_string(_state)
			+ " (expected state=" + std::to_string(expected_state)
			+ ") to state=" + std::to_string(state);
		throw std::runtime_error(error);
	}
	if (_state == expected_state) {
		_state = state;
		_state_changed.notify_one();
	}
}

void ServiceState::change_unless(int state, int not_state, bool throw_ex)
{
	std::unique_lock<std::mutex> mlock(_state_mutex);
	if ((_state == not_state) && throw_ex) {
		std::string error = std::string("change_unless failed changing state=") + std::to_string(_state)
			+ " (not expected) to state=" + std::to_string(state);
		throw std::runtime_error(error);
	}
	if (_state != not_state) {
		_state = state;
		_state_changed.notify_one();
	}
}

void ServiceState::wait_for(int state, std::chrono::milliseconds timeout)
{
	std::unique_lock<std::mutex> mlock(_state_mutex);
	std::chrono::milliseconds remainingInterval = timeout;
	std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
	while (state != _state)
	{
		auto result = _state_changed.wait_for(mlock, remainingInterval);
		if (result == std::cv_status::timeout) {
			break;
		}
		else if (state != _state) {
			// The thread may also be unblocked spuriously, without timeout or notify_one().
			// In this case, just continue
			std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
			auto elapsed = currentTime - startTime;
			remainingInterval -= std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
		}
	}
}

} // namespace
} // namespace
