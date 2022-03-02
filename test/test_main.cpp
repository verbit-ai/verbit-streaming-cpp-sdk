#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

/**
 * `CppUnit` main entry point.
 *
 * Link with `CppUnit` `.o` files to produce a binary that tests just those classes.
 */
int main(int argc, char** argv)
{
	CppUnit::TextUi::TestRunner runner;
	CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
	runner.addTest(registry.makeTest());
	runner.run();
	// unfortunately `run()` doesn't return success/fail value
	return 0;
}
