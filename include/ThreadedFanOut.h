/*
 * ThreadedFanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_THREADED_FAN_OUT_H
#define CHIMERATK_THREADED_FAN_OUT_H

#include <ChimeraTK/NDRegisterAccessor.h>
#include <ChimeraTK/ReadAnyGroup.h>

#include "Application.h"
#include "FanOut.h"
#include "InternalModule.h"

namespace ChimeraTK {

  /** FanOut implementation with an internal thread which waits for new data which
   * is read from the given feeding implementation and distributed to any number
   * of slaves. */
  template<typename UserType>
  class ThreadedFanOut : public FanOut<UserType>, public InternalModule {
   public:
    ThreadedFanOut(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> feedingImpl, VariableNetwork& network,
        ConsumerImplementationPairs<UserType> const& consumerImplementationPairs)
    : FanOut<UserType>(feedingImpl), _network(network) {
      assert(feedingImpl->getAccessModeFlags().has(AccessMode::wait_for_new_data));
      for(auto el : consumerImplementationPairs) {
        FanOut<UserType>::addSlave(el.first, el.second);
      }
    }

    ~ThreadedFanOut() { deactivate(); }

    void activate() override {
      assert(!_thread.joinable());
      _thread = boost::thread([this] { this->run(); });
    }

    void deactivate() override {
      if(_thread.joinable()) {
        _thread.interrupt();
        FanOut<UserType>::interrupt();
        _thread.join();
      }
      assert(!_thread.joinable());
    }

    /** Synchronise feeder and the consumers. This function is executed in the
     * separate thread. */
    virtual void run() {
      Application::registerThread("ThFO" + FanOut<UserType>::impl->getName());
      Application::testableModeLock("start");
      testableModeReached = true;

      ChimeraTK::VersionNumber version{nullptr};
      version = readInitialValues();
      while(true) {
        // send out copies to slaves
        Profiler::startMeasurement();
        boost::this_thread::interruption_point();
        auto validity = FanOut<UserType>::impl->dataValidity();
        for(auto& slave : FanOut<UserType>::slaves) {
          // do not send copy if no data is expected (e.g. trigger)
          if(slave->getNumberOfSamples() != 0) {
            slave->accessChannel(0) = FanOut<UserType>::impl->accessChannel(0);
          }
          slave->setDataValidity(validity);
          bool dataLoss = slave->writeDestructively(version);
          if(dataLoss) Application::incrementDataLossCounter();
        }
        // receive data
        boost::this_thread::interruption_point();
        Profiler::stopMeasurement();
        FanOut<UserType>::impl->read();
        version = FanOut<UserType>::impl->getVersionNumber();
      }
    }

    VersionNumber readInitialValues() {
      Application::testableModeUnlock("readInitialValues");
      FanOut<UserType>::impl->read();
      if(!Application::testableModeTestLock()) {
        Application::testableModeLock("readInitialValues");
      }
      return FanOut<UserType>::impl->getVersionNumber();
    }

   protected:
    /** Thread handling the synchronisation, if needed */
    boost::thread _thread;

    /** Reference to VariableNetwork which is being realised by this FanOut. **/
    VariableNetwork& _network;
  };

  /********************************************************************************************************************/

  /** Same as ThreadedFanOut but with return channel */
  template<typename UserType>
  class ThreadedFanOutWithReturn : public ThreadedFanOut<UserType> {
   public:
    ThreadedFanOutWithReturn(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> feedingImpl,
        VariableNetwork& network, ConsumerImplementationPairs<UserType> const& consumerImplementationPairs)
    : ThreadedFanOut<UserType>(feedingImpl, network, consumerImplementationPairs) {
      for(auto el : consumerImplementationPairs) {
        //TODO Calling a virtual in the constructor seems odd,
        //     but works because we want this version's implementation
        addSlave(el.first, el.second);
      }
    }

    void setReturnChannelSlave(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> returnChannelSlave) {
      _returnChannelSlave = returnChannelSlave;
    }

    void addSlave(
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> slave, VariableNetworkNode& consumer) override {
      // TODO Adding slaves is currently by done by the ThreadedFanOut base class.
      //      Refactor constructors and addSlaves for all FanOuts?
      //FanOut<UserType>::addSlave(slave, consumer);
      if(consumer.getDirection().withReturn) {
        assert(_returnChannelSlave == nullptr);
        _returnChannelSlave = slave;
      }
    }

    void run() override {
      Application::registerThread("ThFO" + FanOut<UserType>::impl->getName());
      Application::testableModeLock("start");
      testableModeReached = true;

      TransferElementID var;
      ChimeraTK::VersionNumber version{nullptr};

      version = readInitialValues();

      ReadAnyGroup group({FanOut<UserType>::impl, _returnChannelSlave});
      while(true) {
        // send out copies to slaves
        for(auto& slave : FanOut<UserType>::slaves) {
          // do not feed back value returnChannelSlave if it was received from it
          if(slave->getId() == var) continue;
          // do not send copy if no data is expected (e.g. trigger)
          if(slave->getNumberOfSamples() != 0) {
            slave->accessChannel(0) = FanOut<UserType>::impl->accessChannel(0);
          }
          bool dataLoss = slave->writeDestructively(version);
          if(dataLoss) Application::incrementDataLossCounter();
        }
        // receive data
        boost::this_thread::interruption_point();
        Profiler::stopMeasurement();
        var = group.readAny();
        Profiler::startMeasurement();
        boost::this_thread::interruption_point();
        // if the update came through the return channel, return it to the feeder
        if(var == _returnChannelSlave->getId()) {
          FanOut<UserType>::impl->accessChannel(0).swap(_returnChannelSlave->accessChannel(0));
          if(version < _returnChannelSlave->getVersionNumber()) {
            version = _returnChannelSlave->getVersionNumber();
          }
          FanOut<UserType>::impl->write(version);
        }
        else {
          version = FanOut<UserType>::impl->getVersionNumber();
        }
      }
    }

   protected:
    /** Thread handling the synchronisation, if needed */
    boost::thread _thread;

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> _returnChannelSlave;

    using ThreadedFanOut<UserType>::_network;
    using ThreadedFanOut<UserType>::readInitialValues;
    using EntityOwner::testableModeReached;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_THREADED_FAN_OUT_H */
