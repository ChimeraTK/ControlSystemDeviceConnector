/*
 * ImplementationAdapter.h
 *
 *  Created on: Jun 16, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_IMPLEMENTATION_ADAPTER_H
#define CHIMERATK_IMPLEMENTATION_ADAPTER_H

#include <thread>

#include <ControlSystemAdapter/ProcessScalar.h>

namespace ChimeraTK {

  /** Stupid base class just to be able to put the adapters into a list.
   *  @todo TODO find a better name! */
  class ImplementationAdapterBase {
    public:
      virtual ~ImplementationAdapterBase(){}

      /** Activate synchronisation thread if needed */
      virtual void activate() {}
  };

  /** Adapts two variable implementations (i.e. two ProcessVariables) so they can be connected together. This is needed
   *  e.g. to connect a device register directly with a control system adapter variable without an involved
   *  application accessor.
   *  @todo TODO find a better name!
   *  @todo TODO find a more efficient implementation not requiring each one thread per instance! */
  template<typename UserType>
  class ImplementationAdapter : public ImplementationAdapterBase {

    public:

      ImplementationAdapter(boost::shared_ptr<mtca4u::ProcessVariable> sender,
          boost::shared_ptr<mtca4u::ProcessVariable> receiver)
      {
        _sender = boost::dynamic_pointer_cast<mtca4u::ProcessScalar<UserType>>(sender);
        _receiver = boost::dynamic_pointer_cast<mtca4u::ProcessScalar<UserType>>(receiver);
        assert(_sender && _receiver);
        _thread = std::thread([this] { this->run(); });
      }

    protected:

      /** Synchronise sender and receiver. This function is executed in the separate thread. */
      void run() {
        while(true) {
          while(!_receiver->receive()) std::this_thread::yield();
          _sender->set(_receiver->get());
          _sender->send();
        }
      }

      /** Sender and receiver process variables */
      boost::shared_ptr<mtca4u::ProcessScalar<UserType>> _sender;
      boost::shared_ptr<mtca4u::ProcessScalar<UserType>> _receiver;

      /** Thread handling the synchronisation */
      std::thread _thread;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_IMPLEMENTATION_ADAPTER_H */