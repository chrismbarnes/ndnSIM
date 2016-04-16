/*
 * betweeness.h
 *
 *  Created on: Apr 16, 2016
 *      Author: chris
 */

#ifndef SRC_NDNSIM_MODEL_FW_BETWEENESS_H_
#define SRC_NDNSIM_MODEL_FW_BETWEENESS_H_

#include "ns3/ndn-forwarding-strategy.h"

namespace ns3 {
namespace ndn {
namespace fw {

class Betweeness : public ForwardingStrategy {

private:
	typedef ForwardingStrategy super;

public:
	static TypeId
	GetTypeId();

	Betweeness();

	virtual void
	OnInterest(Ptr<Face> face, Ptr<Interest> interest);

	virtual void
	OnData(Ptr<Face> face, Ptr<Data> data);

protected:
	virtual bool
	DoPropagateInterest(Ptr<Face> inFace, Ptr<const Interest> interest, Ptr<pit::Entry> pitEntry);
};

}
}
} //end of namespaces




#endif /* SRC_NDNSIM_MODEL_FW_BETWEENESS_H_ */
