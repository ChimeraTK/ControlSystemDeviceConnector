#ifndef VERSIONNUMBERUPDATINGREGISTERDECORATOR_H
#define VERSIONNUMBERUPDATINGREGISTERDECORATOR_H

#include <ChimeraTK/NDRegisterAccessorDecorator.h>

/********************************************************************************************************************/

namespace ChimeraTK {

  class EntityOwner;

  /**
   *  NDRegisterAccessorDecorator which sets the current version number of the owning ApplicationModule in postRead. At
   *  the same time it will also propagate the DataValidity flag to/from the owning module.
   */
  template<typename T>
  class VersionNumberUpdatingRegisterDecorator : public NDRegisterAccessorDecorator<T, T> {
   public:
    VersionNumberUpdatingRegisterDecorator(const boost::shared_ptr<NDRegisterAccessor<T>>& target, EntityOwner* owner)
    : NDRegisterAccessorDecorator<T, T>(target), _owner(owner) {}

    void doPostRead() override;
    void doPreWrite() override;

   protected:
    EntityOwner* _owner;

    /** value of validity flag from last read operation */
    DataValidity lastValidity{DataValidity::ok};
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(VersionNumberUpdatingRegisterDecorator);

} /* namespace ChimeraTK */

#endif // VERSIONNUMBERUPDATINGREGISTERDECORATOR_H
