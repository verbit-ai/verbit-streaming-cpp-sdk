#include <string>
#include <mutex>

namespace verbit {
namespace streaming {

/**
 * Class to track the state of the Verbit Transcribe Streaming service.
 */
class ServiceState {
public:
	/// Construct a new service state.
	constexpr ServiceState() : _state(state_initial) { }

	static const int state_initial = 0;  ///< initial state
	static const int state_opening = 1;  ///< WebSocket is being opened
	static const int state_open = 2;     ///< WebSocket opened successfully
	static const int state_closing = 3;  ///< sent end-of-stream event; awaiting final responses
	static const int state_done = 4;     ///< final state: session completed successfully
	static const int state_fail = 5;     ///< final state: session failed; end-of-stream event **not** sent

	/// Return the current service state.
	constexpr int get() { return _state; }

	/// Return the current service state as a C string.
	const char* const c_str();

	/// Is the current service state a final state?
	constexpr bool is_final() { return (_state == state_closing) || (_state == state_done) || (_state == state_fail); }

	/// Change state unconditionally.
	///
	/// \param state the state to change to
	void change(int state);

	/// Change state **only if** currently in expected state.
	///
	/// \param state the state to change to
	/// \param expected_state the expected current state
	/// \param throw_ex if true, throw exception if not currently in expected state; otherwise fail silently
	void change_if(int state, int expected_state, bool throw_ex);

	/// Change state **unless** currently in not-expected state.
	///
	/// \param state the state to change to
	/// \param not_state the state not expected to be current state
	/// \param throw_ex if true, throw exception if currently in the not-expected state; otherwise fail silently
	void change_unless(int state, int not_state, bool throw_ex);

private:
	int _state;
	std::mutex _state_mutex;
};

} // namespace
} // namespace
