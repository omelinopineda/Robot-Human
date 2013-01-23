#ifndef ATOMIC_CONSTRAINT_H
#define ATOMIC_CONSTRAINT_H

#include <boost/shared_ptr.hpp>
#include <affordance/AffordanceState.h>
#include "ManipulationRelation.h"

namespace action_authoring
{

/**todo: add comment*/
class AtomicConstraint
{
  
    //------------fields
 private:
    ManRelPtr _relation;

    //------------Constructor--------
 public:
    AtomicConstraint(ManRelPtr relation);

    //---------------Accessors
    ManRelPtr getRelation() { return _relation; }

    //mutators
    void setRelation(ManRelPtr relation) { _relation = relation; }
  
};  //class AtomicConstraint
 
 typedef boost::shared_ptr<AtomicConstraint> AtomicConstraintPtr;
 typedef boost::shared_ptr<const AtomicConstraint> AtomicConstraintConstPtr;

} //namespace action_authroing


#endif //ATOMIC_CONSTRAINT_H
