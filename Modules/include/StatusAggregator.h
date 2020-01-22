#ifndef CHIMERATK_STATUS_AGGREGATOR_H
#define CHIMERATK_STATUS_AGGREGATOR_H

#include "ApplicationModule.h"
#include "ModuleGroup.h"
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
  class StatusAggregator : public ApplicationModule {
  public:

    StatusAggregator(EntityOwner* owner, const std::string& name, const std::string& description,
                     const std::string& output,
                     HierarchyModifier modifier, const std::unordered_set<std::string>& tags = {})
      : ApplicationModule(owner, name, description, modifier, tags), status(this, output, "", "", {})  {
      populateStatusInput();
    }

    ~StatusAggregator() override {}

  protected:

    void mainLoop() override {

//      while(true){
//      // Status collection goes here
//      }
    }

    /// Recursivly search for StatusMonitors and other StatusAggregators
    void populateStatusInput(){

      std::cout << "Populating aggregator " << getName() << std::endl;

      auto subModuleList = getOwner()->findTag(".*").getSubmoduleList();
      auto myAccessorList = getOwner()->findTag(".*").getAccessorList();

      // This does not work, we need to operate on the virtual hierarchy
      //std::list<Module*> subModuleList{getOwner()->getSubmoduleList()};

      for(auto module : subModuleList){

        std::cout << "Testing Module " << module->getName() << std::endl;

        auto accList = module->getAccessorList();
        for(auto it=accList.cbegin(); it!=accList.cend(); it++){
          auto nameFromAcc = it->getOwningModule()->getName();
          std::cout << "  Module owning accessor:" << nameFromAcc << std::endl;
        }

        if(dynamic_cast<StatusMonitor*>(module)){
          std::cout << "  Found Monitor " << module->getName() << std::endl;
        }
        else if(dynamic_cast<StatusAggregator*>(module)){
            std::string moduleName = module->getName();

            std::cout << "Found Aggregator " << moduleName << std::endl;

            statusInput.emplace_back(this, moduleName, "", "");
        }
        else if(dynamic_cast<ModuleGroup*>(module)){
          std::cout << "Found ModuleGroup " << module->getName() << std::endl;
        }
        else{}
      }
      std::cout << std::endl << std::endl;
    }

    /**One of four possible states to be reported*/
    ScalarOutput<uint16_t> status;

    /**Vector of status inputs */
    std::vector<ScalarPushInput<uint16_t>> statusInput{};

    //TODO Also provide this for the aggregator?
//    /** Disable the monitor. The status will always be OFF. You don't have to connect this input.
//     *  When there is no feeder, ApplicationCore will connect it to a constant feeder with value 0, hence the monitor is always enabled.
//     */
//    ScalarPushInput<int> disable{this, "disable", "", "Disable the status monitor"};

  };

} // namespace ChimeraTK
#endif // CHIMERATK_STATUS_AGGREGATOR_H
