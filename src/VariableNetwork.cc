/*
 * VariableNetwork.cc
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 */

#include <libxml++/libxml++.h>

#include "VariableNetwork.h"
#include "Accessor.h"
#include "Application.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  bool VariableNetwork::hasAppNode(AccessorBase *a, AccessorBase *b) const {
    // search for a and b in the nodeList
    size_t c = std::count_if( nodeList.begin(), nodeList.end(),
        [a,b](const VariableNetworkNode n) {
          if(n.getType() != NodeType::Application) return false;
          return a == &(n.getAppAccessor()) || ( b != nullptr && b == &(n.getAppAccessor()) );
        } );

    if(c > 0) return true;
    return false;
  }

  /*********************************************************************************************************************/

  bool VariableNetwork::hasFeedingNode() const {
    auto n = std::count_if( nodeList.begin(), nodeList.end(),
        [](const VariableNetworkNode n) {
          return n.getDirection() == VariableDirection::feeding;
        } );
    assert(n < 2);
    return n == 1;
  }

  /*********************************************************************************************************************/

  size_t VariableNetwork::countConsumingNodes() const {
    return nodeList.size() - (hasFeedingNode() ? 1 : 0);
  }

  /*********************************************************************************************************************/

  size_t VariableNetwork::countFixedImplementations() const {
    return count_if( nodeList.begin(), nodeList.end(),
        [](const VariableNetworkNode n) {
          return n.hasImplementation();
        } );
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addNode(VariableNetworkNode &a) {
    if(a.hasOwner()) {  // already in the network
      assert( &(a.getOwner()) == this );    /// @todo TODO merge networks?
      return;
    }

    // change owner of the node: erase from Application's unconnectedNodeList and set this as owner
    a.setOwner(this);

    // if node is feeding, save as feeder for this network
    if(a.getDirection() == VariableDirection::feeding) {
      // make sure we only have one feeding node per network
      if(hasFeedingNode()) {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
            "Trying to add a feeding accessor to a network already having a feeding accessor.");
      }
      // update value type
      if(a.getValueType() != typeid(AnyType)) valueType = &(a.getValueType());
      // update engineering unit
      if(a.getUnit() != "arbitrary") engineeringUnit = a.getUnit();
    }
    // add node to node list
    nodeList.push_back(a);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addConsumingPublication(const std::string& name) {
    VariableNetworkNode node(name, VariableDirection::consuming);
    node.setOwner(this);
    nodeList.push_back(node);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addFeedingPublication(AccessorBase &a, const std::string& name) {
    addFeedingPublication(a.getValueType(), a.getUnit(), name);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addFeedingPublication(const std::type_info &typeInfo, const std::string& unit, const std::string& name) {
    if(hasFeedingNode()) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Trying to add control-system-to-device publication to a network already having a feeding accessor.");
    }
    auto feeder = VariableNetworkNode(name, VariableDirection::feeding);
    feeder.setOwner(this);
    nodeList.push_back(feeder);
    valueType = &typeInfo;
    engineeringUnit = unit;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addConsumingDeviceRegister(const std::string &deviceAlias, const std::string &registerName) {
    VariableNetworkNode node(deviceAlias, registerName, UpdateMode::push, VariableDirection::consuming);
    node.setOwner(this);
    nodeList.push_back(node);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addFeedingDeviceRegister(AccessorBase &a, const std::string &deviceAlias,
      const std::string &registerName, UpdateMode mode) {
    addFeedingDeviceRegister(a.getValueType(), a.getUnit(), deviceAlias, registerName, mode);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addFeedingDeviceRegister(const std::type_info &typeInfo, const std::string& unit
      , const std::string &deviceAlias, const std::string &registerName, UpdateMode mode) {
    if(hasFeedingNode()) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Trying to add a feeding device register to a network already having a feeding accessor.");
    }
    auto feeder = VariableNetworkNode(deviceAlias, registerName, mode, VariableDirection::feeding);
    feeder.setOwner(this);
    nodeList.push_back(feeder);
    valueType = &typeInfo;
    engineeringUnit = unit;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::dump(const std::string& linePrefix) const {
    std::cout << linePrefix << "VariableNetwork {" << std::endl;
    std::cout << linePrefix << "  value type = " << valueType->name() << ", engineering unit = " << engineeringUnit << std::endl;
    std::cout << linePrefix << "  trigger type = ";
    try {
      TriggerType tt = getTriggerType();
      if(tt == TriggerType::feeder) std::cout << "feeder" << std::endl;
      if(tt == TriggerType::pollingConsumer) std::cout << "pollingConsumer" << std::endl;
      if(tt == TriggerType::external) std::cout << "external" << std::endl;
      if(tt == TriggerType::none) std::cout << "none" << std::endl;
    }
    catch(ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork> &e) {
      std::cout << "**error**" << std::endl;
    }
    std::cout << linePrefix << "  feeder";
    getFeedingNode().dump();
    std::cout << linePrefix << "  consumers: " << countConsumingNodes() << std::endl;
    size_t count = 0;
    for(auto &consumer : nodeList) {
      if(consumer.getDirection() != VariableDirection::consuming) continue;
      std::cout << linePrefix << "    # " << ++count << ":";
      consumer.dump();
    }
    if(hasExternalTrigger) {
      std::cout << linePrefix << "  external trigger network:" << std::endl;;
      assert(externalTrigger != nullptr);
      externalTrigger->dump("    ");
    }
    std::cout << linePrefix << "}" << std::endl;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addTriggerReceiver(VariableNetwork *network) {
    VariableNetworkNode node(network);
    node.setOwner(this);
    nodeList.push_back(node);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addTrigger(VariableNetwork &trigger) {

    if(hasExternalTrigger) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Only one external trigger per variable network is allowed.");
    }

    // add ourselves as a trigger receiver to the other network
    trigger.addTriggerReceiver(this);

    // set flag and store pointer to other network
    hasExternalTrigger = true;
    externalTrigger = &trigger;

  }

  /*********************************************************************************************************************/

  void VariableNetwork::addTrigger(VariableNetworkNode trigger) {

    if(hasExternalTrigger) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Only one external trigger per variable network is allowed.");
    }

    // add ourselves as a trigger receiver to the other network
    trigger.getOwner().addTriggerReceiver(this);

    // set flag and store pointer to other network
    hasExternalTrigger = true;
    externalTrigger = &trigger.getOwner();

  }
  /*********************************************************************************************************************/

  VariableNetwork::TriggerType VariableNetwork::getTriggerType() const {
    const auto &feeder = getFeedingNode();
    // network has an external trigger
    if(hasExternalTrigger) {
      if(feeder.getMode() == UpdateMode::push) {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
            "Providing an external trigger to a variable network which is fed by a pushing variable is not allowed.");
      }
      return TriggerType::external;
    }
    // network is fed by a pushing node
    if(feeder.getMode() == UpdateMode::push) {
      return TriggerType::feeder;
    }
    // network is fed by a poll-type node: must have exactly one polling consumer
    size_t nPollingConsumers = count_if( nodeList.begin(), nodeList.end(),
        [](const VariableNetworkNode n) {
          return n.getDirection() == VariableDirection::consuming && n.getMode() == UpdateMode::poll;
        } );
    if(nPollingConsumers != 1) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "In a network with a poll-type feeder and no external trigger, there must be exactly one polling consumer.");
    }
    return TriggerType::pollingConsumer;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::check() {
    // must have consuming nodes
    if(countConsumingNodes() == 0) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "No consuming nodes connected to this network!");
    }

    // must have a feeding node
    if(!hasFeedingNode()) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "No feeding node connected to this network!");
    }

    dump();

    // the network's value type must be correctly set
    if(*valueType == typeid(AnyType)) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "No data type specified for any of the nodes in this network!");
    }

    // all nodes must have this network as the owner and a value type equal the network's value type
    for(auto &node : nodeList) {
      assert(&(node.getOwner()) == this);
      if(node.getValueType() == typeid(AnyType)) node.setValueType(*valueType);
      assert(node.getValueType() == *valueType);
    }

    // if the feeder is an application node, it must be in push mode
    if(getFeedingNode().getType() == NodeType::Application) {
      assert(getFeedingNode().getMode() == UpdateMode::push);
    }

    // check if trigger is correctly defined (the return type doesn't matter, only the checks done in the function are needed)
    getTriggerType();
  }

  /*********************************************************************************************************************/

  VariableNetwork& VariableNetwork::getExternalTrigger() {
    if(getTriggerType() != TriggerType::external) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
          "VariableNetwork::getExternalTrigger() may only be called if the trigger type is external.");
    }
    return *externalTrigger;
  }

  /*********************************************************************************************************************/

  VariableNetworkNode VariableNetwork::getFeedingNode() const {
    auto iter = std::find_if( nodeList.begin(), nodeList.end(),
        [](const VariableNetworkNode n) {
          return n.getDirection() == VariableDirection::feeding;
        } );
    assert(iter != nodeList.end());
    return *iter;
  }

  /*********************************************************************************************************************/

  std::list<VariableNetworkNode> VariableNetwork::getConsumingNodes() const{
    std::list<VariableNetworkNode> consumers;
    for(auto &n : nodeList) if(n.getDirection() == VariableDirection::feeding) consumers.push_back(n);
    return consumers;
  }

}