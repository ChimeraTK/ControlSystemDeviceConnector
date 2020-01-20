
#define BOOST_TEST_MODULE testStatusAggregator
#include <boost/test/included/unit_test.hpp>

#include "StatusAggregator.h"

#include "Application.h"
#include "TestFacility.h"


using namespace boost::unit_test_framework;

namespace ctk = ChimeraTK;


struct TestApplication : public ctk::Application {
  TestApplication() : Application("testApp"){}
  ~TestApplication(){ shutdown(); }

  ctk::StatusAggregator globalStatusAggregator{this, "globalStatusAggregator", "Global StatusAggregator of testApp",
                                               ChimeraTK::HierarchyModifier::none, {"STATUS"}};
};
