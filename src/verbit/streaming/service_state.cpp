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
	}
}

} // namespace
} // namespace
