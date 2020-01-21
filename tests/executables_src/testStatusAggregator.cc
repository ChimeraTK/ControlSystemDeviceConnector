
#define BOOST_TEST_MODULE testStatusAggregator
#include <boost/test/included/unit_test.hpp>

#include "StatusAggregator.h"

#include "Application.h"
#include "ModuleGroup.h"
#include "TestFacility.h"


using namespace boost::unit_test_framework;

namespace ctk = ChimeraTK;


struct OuterGroup : public ctk::ModuleGroup {
  using ctk::ModuleGroup::ModuleGroup;
  virtual ~OuterGroup(){}

  ctk::MinMonitor<double_t> outerMinMonitor{this, "outerMinMonitor", "", "watch", "status", ctk::HierarchyModifier::none,
                           {"OUTER_MON_OUTPUT"}, {"OUTER_MON_PARAMS"},{"OUTER_MON_INPUT"}};

  //ctk::StatusAggregator outerStatusAggregator{this, "outerStatusAggregator", "blah", ctk::HierarchyModifier::none, {"STATUS"}};

  struct InnerGroup : public ctk::ModuleGroup {
    using ctk::ModuleGroup::ModuleGroup;

    ctk::MinMonitor<double_t> innerMinMonitor{this, "innerMinMonitor", "", "minWatch", "minStatus", ctk::HierarchyModifier::none,
                             {"INNER_MON_OUTPUT"}, {"INNER_MON_PARAMS"},{"INNER_MON_INPUT"}};
    ctk::StateMonitor<uint8_t> innerMaxMonitor{this, "innerStateMonitor", "", "stateWatch", "stateStatus", ctk::HierarchyModifier::none,
                             {"INNER_MON_OUTPUT"}, {"INNER_MON_PARAMS"},{"INNER_MON_INPUT"}};

  } innerGroup{this, "innerModuleGroup", ""};

};


struct TestApplication : public ctk::Application {
  TestApplication() : Application("testApp"){}
  ~TestApplication(){ shutdown(); }

  OuterGroup outerModuleGroup1{this, "outerModuleGroup1", "", ctk::HierarchyModifier::none, {}};
  OuterGroup outerModuleGroup2{this, "outerModuleGroup2", ""};


  ctk::StatusAggregator globalStatusAggregator{this, "globalStatusAggregator", "Global StatusAggregator of testApp",
                                               ctk::HierarchyModifier::none, {"STATUS"}};

  ctk::ControlSystemModule cs;

  void defineConnections(){
    findTag(".*").connectTo(cs);
  }
};



BOOST_AUTO_TEST_CASE(testStatusAggregator){
  std::cout << "testStatusAggregator" << std::endl;

  TestApplication app;

  ctk::TestFacility test;
  test.runApplication();
  //app.dump();

}
