#include <iostream>

#include "response_type_test.h"

using namespace verbit::streaming;

CPPUNIT_TEST_SUITE_REGISTRATION(ResponseTypeTest);

void ResponseTypeTest::test_default_ctor()
{
	ResponseType rt = ResponseType();
	CPPUNIT_ASSERT_MESSAGE("default ctor types", rt.types() == ResponseType::Captions);
}

void ResponseTypeTest::test_int_ctor()
{
	ResponseType rt = ResponseType(ResponseType::Captions);
	CPPUNIT_ASSERT_MESSAGE("int ctor (1) types", rt.types() == ResponseType::Captions);
	rt = ResponseType(ResponseType::Transcript | ResponseType::Captions);
	CPPUNIT_ASSERT_MESSAGE("int ctor (2) types", rt.types() == (ResponseType::Transcript | ResponseType::Captions));
}

void ResponseTypeTest::test_string_ctor()
{
	ResponseType rt = ResponseType(std::string("Captions,Transcript"));
	CPPUNIT_ASSERT_MESSAGE("string ctor (2) types", rt.types() == (ResponseType::Transcript | ResponseType::Captions));
	rt = ResponseType(std::string("Transcript"));
	CPPUNIT_ASSERT_MESSAGE("string ctor (1) types", rt.types() == ResponseType::Transcript);
}

void ResponseTypeTest::test_string_ctor_invalid()
{
	CPPUNIT_ASSERT_THROW_MESSAGE("string ctor with invalid type", ResponseType(std::string("Gibberish")), std::runtime_error);
}

void ResponseTypeTest::test_to_string()
{
	ResponseType rt = ResponseType(ResponseType::Captions);
	CPPUNIT_ASSERT_MESSAGE("to_string (one)", rt.to_string() == "Captions");
	rt = ResponseType(ResponseType::Transcript | ResponseType::Captions);
	CPPUNIT_ASSERT_MESSAGE("to_string (two)", rt.to_string() == "Transcript,Captions");
}

void ResponseTypeTest::test_to_string_none()
{
	ResponseType rt = ResponseType(0);
	CPPUNIT_ASSERT_MESSAGE("to_string (zero)", rt.to_string() == "None");
}

void ResponseTypeTest::test_to_string_invalid()
{
	ResponseType rt = ResponseType(0xff);
	CPPUNIT_ASSERT_THROW_MESSAGE("to_string with invalid types", rt.to_string(), std::runtime_error);
}

void ResponseTypeTest::test_url_params()
{
	ResponseType rt = ResponseType(ResponseType::Captions);
	CPPUNIT_ASSERT_MESSAGE("url_params (captions)", rt.url_params() == "get_transcript=False&get_captions=True");
	rt = ResponseType(ResponseType::Transcript);
	CPPUNIT_ASSERT_MESSAGE("url_params (transcript)", rt.url_params() == "get_transcript=True&get_captions=False");
	rt = ResponseType(ResponseType::Transcript | ResponseType::Captions);
	CPPUNIT_ASSERT_MESSAGE("url_params (transcript + captions)", rt.url_params() == "get_transcript=True&get_captions=True");
}

void ResponseTypeTest::test_eos()
{
	ResponseType rt = ResponseType(ResponseType::Transcript | ResponseType::Captions);
	CPPUNIT_ASSERT_MESSAGE("eos (none received)", rt.is_eos() == false);
	nlohmann::json response = nlohmann::json::parse("{\"type\":\"captions\",\"is_end_of_stream\":false}");
	rt.record_eos(response);
	CPPUNIT_ASSERT_MESSAGE("eos (non-EOS received)", rt.is_eos() == false);
	response = nlohmann::json::parse("{\"type\":\"transcript\",\"is_end_of_stream\":true}");
	rt.record_eos(response);
	CPPUNIT_ASSERT_MESSAGE("eos (only transcript EOS received)", rt.is_eos() == false);
	response = nlohmann::json::parse("{\"type\":\"captions\",\"is_end_of_stream\":true}");
	rt.record_eos(response);
	CPPUNIT_ASSERT_MESSAGE("eos (all EOS received)", rt.is_eos() == true);
}
