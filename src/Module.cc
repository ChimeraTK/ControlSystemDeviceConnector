/*
 * Module.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include "Module.h"
#include "Application.h"
#include "VirtualModule.h"
#include "ApplicationModule.h"

namespace ChimeraTK {

  Module::Module(EntityOwner* owner, const std::string& name, const std::string& description,
      HierarchyModifier hierarchyModifier, const std::unordered_set<std::string>& tags)
  : EntityOwner(name, description, hierarchyModifier, tags), _owner(owner) {
    if(_owner != nullptr) _owner->registerModule(this);
  }

  /*********************************************************************************************************************/

  Module::Module(EntityOwner* owner, const std::string& name, const std::string& description, bool eliminateHierarchy,
      const std::unordered_set<std::string>& tags)
  : EntityOwner(name, description, eliminateHierarchy, tags), _owner(owner) {
    if(_owner != nullptr) _owner->registerModule(this);
  }

  /*********************************************************************************************************************/

  Module::~Module() {
    if(_owner != nullptr) _owner->unregisterModule(this);
  }

  /*********************************************************************************************************************/

  Module& Module::operator=(Module&& other) {
    EntityOwner::operator=(std::move(other));
    _owner = other._owner;
    if(_owner != nullptr) _owner->registerModule(this, false);
    // note: the other module unregisters itself in its destructor - will will be
    // called next after any move operation
    return *this;
  }
  /*********************************************************************************************************************/

  void Module::run() {
    testableModeReached = true; // Modules which don't implement run() have now reached testable mode
  }

  /*********************************************************************************************************************/

  ChimeraTK::ReadAnyGroup Module::readAnyGroup() {
    auto recursiveAccessorList = getAccessorListRecursive();

    // put push-type transfer elements into a ReadAnyGroup
    ChimeraTK::ReadAnyGroup group;
    for(auto& accessor : recursiveAccessorList) {
      if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      group.add(accessor.getAppAccessorNoType());
    }

    group.finalise();
    return group;
  }

  /*********************************************************************************************************************/

  void Module::readAll(bool includeReturnChannels) {
    auto recursiveAccessorList = getAccessorListRecursive();
    // first blockingly read all push-type variables
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getMode() != UpdateMode::push) continue;
      if(includeReturnChannels) {
        if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      }
      else {
        if(accessor.getDirection().dir != VariableDirection::consuming) continue;
      }
      accessor.getAppAccessorNoType().read();
    }
    // next non-blockingly read the latest values of all poll-type variables
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getMode() == UpdateMode::push) continue;
      // poll-type accessors cannot have a readback channel
      if(accessor.getDirection().dir != VariableDirection::consuming) continue;
      accessor.getAppAccessorNoType().readLatest();
    }
  }

  /*********************************************************************************************************************/

  void Module::readAllNonBlocking(bool includeReturnChannels) {
    auto recursiveAccessorList = getAccessorListRecursive();
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getMode() != UpdateMode::push) continue;
      if(includeReturnChannels) {
        if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      }
      else {
        if(accessor.getDirection().dir != VariableDirection::consuming) continue;
      }
      accessor.getAppAccessorNoType().readNonBlocking();
    }
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getMode() == UpdateMode::push) continue;
      // poll-type accessors cannot have a readback channel
      if(accessor.getDirection().dir != VariableDirection::consuming) continue;
      accessor.getAppAccessorNoType().readLatest();
    }
  }

  /*********************************************************************************************************************/

  void Module::readAllLatest(bool includeReturnChannels) {
    auto recursiveAccessorList = getAccessorListRecursive();
    for(auto accessor : recursiveAccessorList) {
      if(includeReturnChannels) {
        if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      }
      else {
        if(accessor.getDirection().dir != VariableDirection::consuming) continue;
      }
      accessor.getAppAccessorNoType().readLatest();
    }
  }

  /*********************************************************************************************************************/

  void Module::writeAll(bool includeReturnChannels) {
    auto versionNumber = getCurrentVersionNumber();
    auto recursiveAccessorList = getAccessorListRecursive();
    for(auto accessor : recursiveAccessorList) {
      if(includeReturnChannels) {
        if(accessor.getDirection() == VariableDirection{VariableDirection::consuming, false}) continue;
      }
      else {
        if(accessor.getDirection().dir != VariableDirection::feeding) continue;
      }
      accessor.getAppAccessorNoType().write(versionNumber);
    }
  }

  /*********************************************************************************************************************/

  void Module::writeAllDestructively(bool includeReturnChannels) {
    auto versionNumber = getCurrentVersionNumber();
    auto recursiveAccessorList = getAccessorListRecursive();
    for(auto accessor : recursiveAccessorList) {
      if(includeReturnChannels) {
        if(accessor.getDirection() == VariableDirection{VariableDirection::consuming, false}) continue;
      }
      else {
        if(accessor.getDirection().dir != VariableDirection::feeding) continue;
      }
      accessor.getAppAccessorNoType().writeDestructively(versionNumber);
    }
  }

  /*********************************************************************************************************************/

  Module& Module::submodule(const std::string& moduleName) const {
    size_t slash = moduleName.find_first_of("/");
    // no slash found: call subscript operator
    if(slash == std::string::npos) return (*this)[moduleName];
    // slash found: split module name at slash
    std::string upperModuleName = moduleName.substr(0, slash);
    std::string remainingModuleNames = moduleName.substr(slash + 1);
    return (*this)[upperModuleName].submodule(remainingModuleNames);
  }

  std::string Module::getVirtualQualifiedName() const {
    std::string virtualQualifiedName{""};
    const EntityOwner* currentLevelModule{this};

    bool rootReached{false};
    do {
      if(currentLevelModule == &Application::getInstance()) {
        break;
      }

      auto currentLevelModifier = currentLevelModule->getHierarchyModifier();
      bool skipNextLevel{false};

      switch(currentLevelModifier) {
        case HierarchyModifier::none:
          virtualQualifiedName = "/" + currentLevelModule->getName() + virtualQualifiedName;
          break;
        case HierarchyModifier::hideThis:
          // Omit name of current level
          break;
        case HierarchyModifier::oneLevelUp:
          virtualQualifiedName = "/" + currentLevelModule->getName() + virtualQualifiedName;
          skipNextLevel = true;
          break;
        case HierarchyModifier::oneUpAndHide:
          skipNextLevel = true;
          break;
        case HierarchyModifier::moveToRoot:
          virtualQualifiedName = "/" + currentLevelModule->getName() + virtualQualifiedName;
          rootReached = true;
          break;
      }

      if(skipNextLevel) {
        auto lastLevelModule = currentLevelModule;
        currentLevelModule = dynamic_cast<const Module*>(currentLevelModule)->getOwner();

        if(currentLevelModule == &Application::getInstance()) {
          throw logic_error(std::string("Module ") + lastLevelModule->getName() +
              ": cannot have hierarchy modifier 'oneLevelUp' or oneUpAndHide in root of the application.");
        }
      }

      if(!rootReached) {
        currentLevelModule = dynamic_cast<const Module*>(currentLevelModule)->getOwner();
      }
    } while(!rootReached);

    if(virtualQualifiedName == "") virtualQualifiedName = "/";

    return virtualQualifiedName;
  }

  /*********************************************************************************************************************/

  ApplicationModule* Module::findApplicationModule() {
    if(getModuleType() == ModuleType::ApplicationModule) {
      auto* ret = dynamic_cast<ApplicationModule*>(this);
      assert(ret != nullptr);
      return ret;
    }
    else if(getModuleType() == ModuleType::VariableGroup) {
      auto* owningModule = dynamic_cast<Module*>(getOwner());
      assert(owningModule != nullptr);
      return owningModule->findApplicationModule();
    }
    else {
      throw ChimeraTK::logic_error(
          "EntityOwner::findApplicationModule() called on neither an ApplicationModule nor a VariableGroup.");
    }
  }

  /*********************************************************************************************************************/

} /* namespace ChimeraTK */
