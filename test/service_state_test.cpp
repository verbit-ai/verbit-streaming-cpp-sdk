#include <iostream>
#include <string.h>

#include "service_state_test.h"

using namespace verbit::streaming;

CPPUNIT_TEST_SUITE_REGISTRATION(ServiceStateTest);

void ServiceStateTest::test_default_ctor()
{
	ServiceState ss;
	CPPUNIT_ASSERT_MESSAGE("default ctor state", ss.get() == ServiceState::state_initial);
}

void ServiceStateTest::test_c_str()
{
	ServiceState ss;
	CPPUNIT_ASSERT_MESSAGE("c_str initial", strcmp("initial", ss.c_str()) == 0);
	ss.change(ServiceState::state_opening);
	CPPUNIT_ASSERT_MESSAGE("c_str opening", strcmp("opening", ss.c_str()) == 0);
}

void ServiceStateTest::test_is_final()
{
	ServiceState ss;
	CPPUNIT_ASSERT_MESSAGE("is_final initial", !ss.is_final());
	ss.change(ServiceState::state_opening);
	CPPUNIT_ASSERT_MESSAGE("is_final opening", !ss.is_final());
	ss.change(ServiceState::state_open);
	CPPUNIT_ASSERT_MESSAGE("is_final open", !ss.is_final());
	ss.change(ServiceState::state_closing);
	CPPUNIT_ASSERT_MESSAGE("is_final closing", ss.is_final());
	ss.change(ServiceState::state_done);
	CPPUNIT_ASSERT_MESSAGE("is_final done", ss.is_final());
	ss.change(ServiceState::state_fail);
	CPPUNIT_ASSERT_MESSAGE("is_final fail", ss.is_final());
}

void ServiceStateTest::test_change()
{
	ServiceState ss;
	ss.change(ServiceState::state_fail);
	CPPUNIT_ASSERT_MESSAGE("change", ss.get() == ServiceState::state_fail);
}

void ServiceStateTest::test_change_if()
{
	ServiceState ss;
	ss.change(ServiceState::state_opening);
	ss.change_if(ServiceState::state_open, ServiceState::state_opening, false);
	CPPUNIT_ASSERT_MESSAGE("change_if", ss.get() == ServiceState::state_open);
	ss.change_if(ServiceState::state_fail, ServiceState::state_initial, false);
	CPPUNIT_ASSERT_MESSAGE("change_if", ss.get() == ServiceState::state_open);
}

void ServiceStateTest::test_change_if_except()
{
	ServiceState ss;
	ss.change(ServiceState::state_open);
	CPPUNIT_ASSERT_THROW_MESSAGE("change_if exception", ss.change_if(ServiceState::state_fail, ServiceState::state_initial, true), std::runtime_error);
}

void ServiceStateTest::test_change_unless()
{
	ServiceState ss;
	ss.change(ServiceState::state_open);
	ss.change_unless(ServiceState::state_done, ServiceState::state_fail, false);
	CPPUNIT_ASSERT_MESSAGE("change_unless", ss.get() == ServiceState::state_done);
	ss.change_unless(ServiceState::state_fail, ServiceState::state_done, false);
	CPPUNIT_ASSERT_MESSAGE("change_unless", ss.get() == ServiceState::state_done);
}

void ServiceStateTest::test_change_unless_except()
{
	ServiceState ss;
	ss.change(ServiceState::state_done);
	CPPUNIT_ASSERT_THROW_MESSAGE("change_unless exception", ss.change_unless(ServiceState::state_fail, ServiceState::state_done, true), std::runtime_error);
}
