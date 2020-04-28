#include "MetaDataPropagatingRegisterDecorator.h"
#include "EntityOwner.h"

namespace ChimeraTK {

  template<typename T>
  void MetaDataPropagatingRegisterDecorator<T>::doPostRead(TransferType type, bool hasNewData) {
    NDRegisterAccessorDecorator<T, T>::doPostRead(type, hasNewData);

    // update the version number
    if(!isNonblockingRead) {
      _owner->setCurrentVersionNumber(this->getVersionNumber());
    }

    // Check if the data validity flag changed. If yes, propagate this information to the owning module.
    auto valid = ChimeraTK::NDRegisterAccessorDecorator<T>::dataValidity();
    if(valid != lastValidity) {
      if(valid == DataValidity::faulty)
        _owner->incrementDataFaultCounter();
      else
        _owner->decrementDataFaultCounter();
      lastValidity = valid;
    }
  }

  template<typename T>
  void MetaDataPropagatingRegisterDecorator<T>::doPreWrite(TransferType type, VersionNumber versionNumber) {
    if(localValidity == DataValidity::faulty)
      ChimeraTK::NDRegisterAccessorDecorator<T>::setDataValidity(DataValidity::faulty);
    else
      ChimeraTK::NDRegisterAccessorDecorator<T>::setDataValidity(_owner->getDataValidity());

    NDRegisterAccessorDecorator<T, T>::doPreWrite(type, versionNumber);
  }

} // namespace ChimeraTK

INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ChimeraTK::MetaDataPropagatingRegisterDecorator);
