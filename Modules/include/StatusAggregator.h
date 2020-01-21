#ifndef CHIMERATK_STATUS_AGGREGATOR_H
#define CHIMERATK_STATUS_AGGREGATOR_H

#include "ApplicationCore.h"
#include "StatusMonitor.h"

#include <list>
#include <vector>
#include <string>

namespace ChimeraTK{

  /**
   * The StatusAggregator collects results of multiple StatusMonitor instances
   * and aggregates them into a single status, which can take the same values
   * as the result of the individual monitors.
   *
   * Note: The aggregated instances are collected on construction. Hence, the
   * StatusAggregator has to be declared after all instances that shall to be
   * included in the scope (ModuleGroup, Application, ...) of interest.
   */
  struct StatusAggregator : public ApplicationModule {

    StatusAggregator(EntityOwner* owner, const std::string& name, const std::string& description,
                     HierarchyModifier modifier, const std::unordered_set<std::string>& tags = {})
    : ApplicationModule(owner, name, description, modifier, tags)  {}

    ~StatusAggregator() override {}

    void mainLoop() override {

      // Search for StatusMonitors and other StatusAggregators
      auto subModuleList = getOwner()->findTag(".*").getSubmoduleList();
      auto myAccessorList = getOwner()->findTag(".*").getAccessorList();

      // This does not work, we need to operate on the virtual hierarchy
      //std::list<Module*> subModuleList{getOwner()->getSubmoduleList()};

      for(auto module : subModuleList){

        if(dynamic_cast<StatusMonitor*>(module)){

        }
        else if(dynamic_cast<StatusAggregator*>(module)){
            std::string name = module->getName();
            statusInput.emplace_back(this, name, "", "");
        }
        else if(dynamic_cast<ModuleGroup*>(module)){
          //recurse
        }
        else{}
      }

//      while(true){
//      // Status collection goes here
//      }
    }

    /**Vector of status inputs */
    std::vector<ScalarPushInput<uint16_t>> statusInput;

    /**One of four possible states to be reported*/
    ScalarOutput<uint16_t> status;

    //TODO Also provide this for the aggregator?
//    /** Disable the monitor. The status will always be OFF. You don't have to connect this input.
//     *  When there is no feeder, ApplicationCore will connect it to a constant feeder with value 0, hence the monitor is always enabled.
//     */
//    ScalarPushInput<int> disable{this, "disable", "", "Disable the status monitor"};

  };

} // namespace ChimeraTK
#endif // CHIMERATK_STATUS_AGGREGATOR_H
